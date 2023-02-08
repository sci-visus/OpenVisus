
# SSH tunneling firewall problem

There are two ways to circumvent firewalling problems.

## SSH dynamic SOCKS (recommended)

Run a local SOCKS server while connecting to the remote (behind a firewall) node:
- `-q` means: quiet
- `-C` means: enable compression
- `-D <port>` create a SOCKS server

```
ssh -D 8888 -q -C <ssh-remote-hostname>
```

Then change your local browser proxy setting.
For example in Firefox `Settings`/`Manual proxy configuration`:

```
SOCKS HOST: localhost
SOCKS PORT: 8888
SOCKS TYPE: SOCKS_v5
PROXY DNS when using SOCKS v5: checked
ENABLE DNS over HTTPS: checked
```

**IMPORTANT TO REMEMBER** When typing the Jupyter/Bokeh url in your browser, make sure always to use **the full qualified hostname** 
since `localhost` or `127.0.0.1` will NOT work (the browser refuses to use any proxy server for any local address).


## SSH local port forwarding

NOTE: With this modality, you should know the ports-to-forward in advance.
Each bokeh app, running in a Jupyter Notebook cell, seems to need two forwarded port.

You can forward ssh-port(s) by doing:

```
ssh \
   -L local-port:127.0.0.1:remote-port \
   -L local-port:127.0.0.1:remote-port \
   ...
```

Or you can change the  `~/.ssh/config` to forward ports:

```
Host our-hostname
	HostName your-hostname
	User your-username
	Port 22	
	ServerAliveInterval 60
	IdentityFile ~/.ssh/your-private-identity
	LocalForward 10011 127.0.0.1:10011
	LocalForward 10012 127.0.0.1:10012          
```
