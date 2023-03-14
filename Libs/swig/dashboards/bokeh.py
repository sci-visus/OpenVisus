import os,sys,logging, urllib
import urllib.request
from pprint import pprint

import bokeh
import bokeh.io 
import bokeh.io.notebook 
import bokeh.models.widgets  
import bokeh.core.validation
import bokeh.plotting
import bokeh.core.validation.warnings
import bokeh.layouts

from . utils import cbool

bokeh.core.validation.silence(bokeh.core.validation.warnings.EMPTY_LAYOUT, True)
bokeh.core.validation.silence(bokeh.core.validation.warnings.FIXED_SIZING_MODE,True)

# //////////////////////////////////////////////////////////////////////////////////////
def ShowApp(app):
    
    # in JypyterLab/JupyterHub we need to tell what is the proxy url
    # see https://docs.bokeh.org/en/3.0.3/docs/user_guide/output/jupyter.html
    # example: 
    VISUS_PUBLIC_IP=cbool(os.environ.get("VISUS_PUBLIC_IP",False))

    # change this if you need ssh-tunneling
    # see https://github.com/sci-visus/OpenVisus/blob/master/docs/ssh-tunnels.md
    VISUS_SSH_TUNNELS=str(os.environ.get("VISUS_SSH_TUNNELS",""))
    
    if VISUS_SSH_TUNNELS:
        notebook_port,bokeh_port=VISUS_SSH_TUNNELS
        print(f"ShowApp, enabling ssh tunnels notebook_port={notebook_port} bokeh_port={bokeh_port}")
        bokeh.io.notebook.show_app(app, bokeh.io.notebook.curstate(), f"http://127.0.0.1:{notebook_port}", port = bokeh_port) 
        
    elif VISUS_PUBLIC_IP:
        
        # retrieve public IP (this is needed for front-end browser to reach bokeh server)
        public_ip = urllib.request.urlopen('https://ident.me').read().decode('utf8')
        print(f"public_ip={public_ip}")    
        
        if "JUPYTERHUB_SERVICE_PREFIX" in os.environ:

            def GetJupyterHubNotebookUrl(port):
                if port is None:
                    ret=public_ip
                    print(f"GetJupyterHubNotebookUrl port={port} returning {ret}")
                    return ret
                else:
                    ret=f"https://{public_ip}{os.environ['JUPYTERHUB_SERVICE_PREFIX']}proxy/{port}"
                    print(f"GetJupyterHubNotebookUrl port={port} returning {ret}")
                    return ret     

            bokeh.io.show(app, notebook_url=GetJupyterHubNotebookUrl)
            
        else:
            # need the port (TODO: test), I assume I will get a non-ambiguos/unique port
            import notebook.notebookapp
            ports=list(set([it['port'] for it in notebook.notebookapp.list_running_servers()]))
            assert(len(ports)==1)
            port=ports[0]
            notebook_url=f"{public_ip}:{port}" 
            print(f"bokeh.io.show(app, notebook_url='{notebook_url}')")
            bokeh.io.show(app, notebook_url=notebook_url)
    else:
        bokeh.io.show(app) 
      
# //////////////////////////////////////////////////////////////////////////////////////
def TestApp(doc):
    fig = bokeh.plotting.figure(title="Multiple line example", x_axis_label="x", y_axis_label="y",height=200)
    fig.line([1, 2, 3, 4, 5], [6, 7, 2, 4, 5])
    grid=bokeh.layouts.grid(children=[fig], nrows=1, ncols=1, sizing_mode='stretch_width') 
    main_layout=bokeh.layouts.column(children=[],sizing_mode='stretch_width')
    main_layout.children.append(grid)  
    button = bokeh.models.widgets.Button(label="Bokeh is working?", sizing_mode='stretch_width')
    def OnClick(evt=None):button.label="YES!"
    button.on_click(OnClick)     
    main_layout.children.append(button)
    doc.add_root(main_layout)  

