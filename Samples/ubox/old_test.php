<html lang="en-US">
<head>
	<meta charset="utf-8" />
	<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
	<meta http-equiv="Pragma" content="no-cache">
	<meta http-equiv="Expires" content="0">
	<title>Ubox PHP Test</title>
</head>
<body>

<?php

# example of installation of GuzzleHttp
# cd /var/www/html 
# sudo composer require guzzlehttp/guzzle
# sudo composer update
# sudo systemctl restart apache2

require './vendor/autoload.php';

use GuzzleHttp\Client;
use GuzzleHttp\RequestOptions;
use GuzzleHttp\Psr7\Request;

session_start();

# ///////////////////////////////////////////////////////////////////////////////////
function GetQueryParam($name,$default="") {

	if (!isset($_GET[$name])) 
		return $default;
	else
		return $_GET[$name];
}


# ///////////////////////////////////////////////////////////////////////////////////
class BoxStorage {

	var $access_token;
	var $refresh_token;

	# __construct
	function __construct($client_id,$client_secret)
	{
		//make sure I have an access token
		if (!isset($_SESSION["access_token"])) 
		{
			$authorization_code=GetQueryParam("code");
			
			# need a redirect to BOX
			if ($authorization_code=="") 
			{
				$protocol=$_SERVER['REQUEST_SCHEME'];
				$server_port=$_SERVER['HTTP_HOST']; # this contains port to, i guess
				$path=parse_url($_SERVER['REQUEST_URI'])["path"];
				$redirect_uri=sprintf("%s://%s%s",$protocol,$server_port,$path);
				header("location: https://app.box.com/api/oauth2/authorize?access_type=offline&response_type=code&client_id=$client_id&redirect_uri=" . http_build_query($redirect_uri));	
				exit;
			}

			$body=sprintf("grant_type=authorization_code&code=%s&client_id=%s&client_secret=%s",$authorization_code,$client_id,$client_secret);
			$response=$this->getNetResponse('POST',"https://api.box.com/oauth2/token",['body'=> $body]);
			$content=json_decode($response,true);
			$_SESSION["access_token" ] = $content["access_token"];
			$_SESSION["refresh_token"] = $content["refresh_token"];
		}

		$this->access_token  = $_SESSION["access_token" ];
		$this->refresh_token = $_SESSION["refresh_token"];
	}

	# getNetResponse
	function getNetResponse($method,$url,$opts) {
		$client = new Client();
		$response = $client->request($method,$url,$opts);
		return $response->getBody()->getContents();
	}

	# getDefaultHeader
	function getDefaultHeader() {
		return ["Authorization" => sprintf("Bearer %s",$this->access_token)];
	}

	# listFolder
	function listFolder($folder_id) 
	{
		$response=$this->getNetResponse("GET","https://api.box.com/2.0/folders/$folder_id",['headers' => $this->getDefaultHeader()]);
		$content=json_decode($response,true);
		$entries=$content["item_collection"]["entries"];
		return $entries;
	}

	# createFolder
	function createFolder($folder_id,$name) 
	{
		$data=sprintf('{"name":"%s", "parent": {"id": "%d"}}',$name,$folder_id);
		$response=$this->getNetResponse("POST","https://api.box.com/2.0/folders",['headers' => $this->getDefaultHeader(),'body'=> $data]);
		$content=json_decode($response,true);
		return $content["id"];
	}

	//removeFolder
	function removeFolder($folder_id) {
		$this->getNetResponse("DELETE","https://api.box.com/2.0/folders/$folder_id?recursive=true",['headers' => $this->getDefaultHeader()]);
	}

	# uploadFile
	function uploadFile($folder_id,$name,$file_content)
	{
		$response = $this->getNetResponse("POST","https://upload.box.com/api/2.0/files/content",[
			'headers' => $this->getDefaultHeader(),
			'multipart' => [
				['name' => 'attributes', 'contents' => sprintf('{"name":"%s", "parent":{"id":"%d"}}',$name,$folder_id)],
				['name' => 'file'      , 'contents' => $file_content]]]);
		$content=json_decode($response,true);
		return $content["entries"][0]["id"];
	}
	
	# downloadFile
	function downloadFile($file_id) 
	{
		$response = $this->getNetResponse("GET","https://api.box.com/2.0/files/$file_id/content",['headers' => $this->getDefaultHeader()]);
		return $response;
	}

	# deleteFile
	function deleteFile($file_id) 
	{
		$this->getNetResponse("DELETE","https://api.box.com/2.0/files/$file_id",['headers' => $this->getDefaultHeader()]);
	}
} 


# todo: move this sensible informations inside a file
$box = new BoxStorage('XXXXXXXXX', 'YYYYYYYYYY');

$action    = GetQueryParam('action');
$folder_id = GetQueryParam('folder_id');

$action="ShowBoxFolderPicker";

?>

<!-- ////////////////////////////////////////////////////////////////////////// -->
<?php
if ($action=="ShowBoxFolderPicker") {

	$body=<<<EOT
		<script src="https://cdn.polyfill.io/v2/polyfill.min.js?features=es6,Intl"></script>
		<link rel="stylesheet" href="https://cdn01.boxcdn.net/platform/elements/10.1.0/en-US/picker.css" />
		<div class="container" style="height:600px"></div>
		<script src="https://cdn01.boxcdn.net/platform/elements/10.1.0/en-US/picker.js"></script>
		<script>
		var picker = new Box.FolderPicker();
		picker.addListener('choose', function(items) {
			folder_id=items[0]["id"];
			console.log("The user clicked on " + folder_id);
			var url = window.location.href;    
			url += (url.indexOf('?') > -1)?"&":"?";
			url += "folder_id=" + folder_id;
			window.location.href = url;
		});
		picker.addListener('cancel', function() {
		  console.log("The user clicked cancel or closed the popup");
		});
		picker.show('0', '$box->access_token', {container: '.container'});
		</script>
EOT;
	print($body);
	exit;
}
?>


<!-- ////////////////////////////////////////////////////////////////////////// -->
<?php
if ($action=="ListFolder") {
	$folder_id=GetQueryParam('folder_id',0);
	$entries=$box->listFolder($folder_id);
	var_dump($entries);
	exit;
}
?>

<!-- ////////////////////////////////////////////////////////////////////////// -->
<?php
if ($action=="TestBoxAPI") {
	$entries=$box->listFolder(0);
	$folder_id=$box->createFolder(0,"test_folder");
	$box->removeFolder($folder_id);
	$file_id=$box->uploadFile(0,"test.png",fopen("/var/www/html/test.png", 'rb'));
	$content=$box->downloadFile($file_id);
	$box->deleteFile($file_id);
	printf("test API ok %s<br>",$_SESSION["access_token"]);
	exit;
}
?>

</body>
</html>




