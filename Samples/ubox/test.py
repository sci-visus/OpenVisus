# NOTE: if you want to use 'pure' python you may want to look for boxsdk for python
# see https://developer.box.com/docs/sdks
# see https://box-content.readme.io/reference

import sys
import json
import urllib.parse
import urllib.request
import http.server
import webbrowser
import requests_toolbelt 
import requests
import collections
import requests_toolbelt.multipart.encoder 

# /////////////////////////////////////////////////////////////////////////////
def LoadFile(filename,mode):
	file = open(filename, mode)
	ret=file.read()
	file.close()
	return ret
	
# /////////////////////////////////////////////////////////////////////////////
def SaveFile(filename,content,mode):
	file = open(filename, mode)
	file.write(content)
	file.close()


# /////////////////////////////////////////////////////////////////////////////
class BoxStorage:

	# constructor
	def __init__(self,client_id="",client_secret="",redirect_uri=""):
		self.client_id=client_id
		self.client_secret=client_secret
		self.redirect_uri=redirect_uri
		self.authorization_code=""
		self.access_token=""
		# enableDebug()
		self.getAuthorizationCode()
		self.getAccessToken()
		
	# getNetResponse
	def getNetResponse(self,method,url,headers={},data=None):
		
		if self.access_token:
			headers["Authorization"]="Bearer " + self.access_token
		
		if method=='post':
			response=requests.post(url,headers=headers,data=data)
		
		elif method=='get':
			response=requests.get(url,headers=headers,data=data)
			
		elif method=='delete':
			response=requests.delete(url,headers=headers,data=data)		
		
		else:
			raise Exception("internal error")
		
		response.raise_for_status()	
		return response
	
	# enableDebug
	def enableDebug(self):
		import logging
		import http.client as http_client
		http_client.HTTPConnection.debuglevel = 1
		logging.basicConfig()
		logging.getLogger().setLevel(logging.DEBUG)
		requests_log = logging.getLogger("requests.packages.urllib3")
		requests_log.setLevel(logging.DEBUG)
		requests_log.propagate = True

	# getAuthorizationCode
	def getAuthorizationCode(self):
		
		authorization_code=""
			
		# get access_token
		# curl https://api.box.com/oauth2/token -d 'grant_type=authorization_code' -d 'code=$authorization_code' -d 'client_id=$client_id' -d 'client_secret=$client_secret' -X POST
		# curl "https://app.box.com/api/oauth2/authorize?access_type=offline&client_id=d0374ba6pgmaguie02ge15sv1mllndho&redirect_uri=http%3A%2F%2F127.0.0.1%3A53682%2F&response_type=code"
		# open a browser and grant access, you will see the code with 'view source' after redirect
		# simply using python is not sufficient since I need to push 'grant button'
		# need to use a browser to show the SSO pages
		class MyRequestHandler(http.server.BaseHTTPRequestHandler):
			def do_GET(self):
				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.end_headers()
				self.server.path = self.path
				nonlocal authorization_code
				authorization_code=urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)['code'][0]

		httpd = http.server.HTTPServer(('', int(self.redirect_uri.split(":")[-1])), MyRequestHandler)
		webbrowser.open("https://app.box.com/api/oauth2/authorize?" + urllib.parse.urlencode({"access_type" : "offline","client_id" : self.client_id,"redirect_uri" : self.redirect_uri,"response_type" : "code"}))
		request=httpd.handle_request()
		self.authorization_code=authorization_code		
		
	
	# getAccessToken
	def getAccessToken(self):
		content=self.getNetResponse('post',"https://api.box.com/oauth2/token", data= {"grant_type":"authorization_code","code":self.authorization_code,"client_id": self.client_id,"client_secret":self.client_secret}).json()
		self.expires_in=content['expires_in']
		self.access_token=content['access_token']
		self.restricted_to=content['restricted_to']
		self.refresh_token=content['refresh_token']
		self.token_type=content['token_type']		
		
	
	# listFolder
	# curl https://api.box.com/2.0/folders/$folder_id -H "Authorization: Bearer $access_token"
	def listFolder(self,folder_id):
		content= self.getNetResponse('get',"https://api.box.com/2.0/folders/%s" % (str(folder_id),)).json()
		ret=[]
		for it in content["item_collection"]["entries"]:
			ret+=[{"id": it["id"],"name" : it["name"], "type" : it["type"]}]
		return ret
		

	# createFolder
	# curl https://api.box.com/2.0/folders -H "Authorization: Bearer $access_token " -d '{"name":"$name", "parent": {"id": "$folder_id"}}' -X POST		
	def createFolder(self,parent_folder_id,name):
		content=self.getNetResponse('post',"https://api.box.com/2.0/folders", data="""{"name":"%s", "parent": {"id": "%s"}}""" % (name,parent_folder_id)).json()
		return content["id"]
		
	# removeFolder
	# curl https://api.box.com/2.0/folders/$folder_id?recursive=true -H "Authorization: Bearer $access_token" -X DELETE
	def removeFolder(self,folder_id):
		self.getNetResponse('delete',"https://api.box.com/2.0/folders/%s?recursive=true" % (folder_id,))

	# uploadFile
	# curl https://upload.box.com/api/2.0/files/content -H "Authorization: Bearer $access_token" -F attributes='{"name":"$filename", "parent":{"id":"0"}}'  -F file=@$filename  -X POST
	def uploadFile(self,folder_id,name,filename,content_type="application/octet-stream"):
		stream=open(filename, 'rb')
		fields = collections.OrderedDict()
		fields['attributes']="""{ "name" : "%s", "parent" : {"id":"%s"} }""" % (name,folder_id)
		fields['file']=(name, stream, content_type)
		data=requests_toolbelt.multipart.encoder.MultipartEncoder(fields)
		content = self.getNetResponse('post',"https://upload.box.com/api/2.0/files/content", headers={'Content-Type':data.content_type}, data=data).json()
		file_id=content["entries"][0]["id"]
		return file_id	
		
	# downloadFile
	# curl -L https://api.box.com/2.0/files/$file_id/content -H "Authorization: Bearer $access_token" -o $filename
	def downloadFile(self,file_id):
		response=self.getNetResponse('get',"https://api.box.com/2.0/files/%s/content" % (file_id,))
		return response.content
		
	# deleteFile
	# curl https://api.box.com/2.0/files/$file_id -H "Authorization: Bearer $access_token" -X DELETE
	def deleteFile(self,file_id):
		self.getNetResponse('delete',"https://api.box.com/2.0/files/%s" % (str(file_id),))

	# createMetadataTemplate
	def createMetadataTemplate(self,template_name,fields):
		self.getNetResponse('post','https://api.box.com/2.0/metadata_templates/schema', 
			data=json.dumps({
			  "scope": 'enterprise', 
			  "templateKey": template_name,
			  "displayName": template_name,
			  "hidden": False,
			  "fields": [ {"key": it,"type": "string","displayName": it} for it in fields]
			}))
		
	# setMetadata
	# curl https://api.box.com/2.0/files/$file_id/metadata/enterprise/marketingCollateral -H "Authorization: Bearer $access_token" -H "Content-Type: application/json" -d '{ "field1": "0", "field2": "1"}' -X POST
	def setMetadata(self,file_id,template_name,metadata):
		self.getNetResponse('post',"https://api.box.com/2.0/files/%s/metadata/enterprise/%s" % (file_id,template_name), headers={"Content-Type" : "application/json"}, data=metadata)

	# getMetadata
	# curl https://api.box.com/2.0/files/$file_id/metadata -H "Authorization: Bearer $access_token"
	def getMetadata(self,file_id,template_name):
		return self.getNetResponse('get',"https://api.box.com/2.0/files/%s/metadata/enterprise/%s" % (file_id,template_name)).json()
	
	# showPicker
	def showPicker(self):
		
		SaveFile("~picker.html","""
		<!DOCTYPE html>
		<html lang="en-US">
		<head>
		    <meta charset="utf-8" />
		    <title>Box Folder Selection</title>
		    <script src="https://cdn.polyfill.io/v2/polyfill.min.js?features=es6,Intl"></script>
		    <link rel="stylesheet" href="https://cdn01.boxcdn.net/platform/elements/6.6.0/en-US/picker.css" />
		</head>
		<body>
		    <div class="container" style="height:600px"></div>
		    <script src="https://cdn01.boxcdn.net/platform/elements/6.6.0/en-US/picker.js"></script>
		    <script>
				var folderPicker = new Box.FolderPicker();
				folderPicker.show('0', '%s', {container: '.container'});
		    </script>
		</body>
		</html>
		""" % (box.access_token,),"wt")

		webbrowser.open("~picker.html")		

# must configure your SCI box account using 'devconsole'# https://uofu.account.box.com/login?redirect_url=%2Fdevelopers%2Fconsole&logout=true	
box=BoxStorage(
	client_id='XXXXXXXXXXXXX',
	client_secret='YYYYYYYYYYYYYY',
	redirect_uri='http://127.0.0.1:53682')
	
root_id=0
entries=box.listFolder(root_id)
folder_id=box.createFolder(0,"test_folder")
box.removeFolder(folder_id)

file_id=box.uploadFile(root_id,"test.png","test.png","image/png")

# metadata BROKEN
# Creating metadata templates is restricted to admins, or co-admins who have been granted rights to Create and edit metadata templates for your company by the admin.
if False:
	box.createMetadataTemplate("OpenVisusMetadataTemplate",("fields1","fields2"))
	box.setMetadata(file_id,"OpenVisusMetadataTemplate",'{"fields1":"0","fields2":"1"}')
	box.getMetadata(file_id,"OpenVisusMetadataTemplate")


content=box.downloadFile(file_id)
SaveFile("~tmp.png",content,"wb")

box.deleteFile(file_id)	

# box.showPicker()








