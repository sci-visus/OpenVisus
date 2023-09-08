
# Connect using a proxy

In your visus.config, you can specify a proxy scheme, ip, and port for the client (either the viewer or the python package) to use when connecting to an OpenViSUS server. This can be useful in accessing a server that is hosted on an internal network which is only accessible through SSH, for example.

An example visus.config file containing proxy information would look like this:

```
<visus>
	<Configuration>
		<NetService proxy="socks5://localhost" proxyport="55051"/>
	</Configuration>
	... (The rest of the config follows)
</visus>
```

The "proxy" variable above contains both the scheme (SOCKS5) and ip (localhost).

A user would need to start the SOCKS5 proxy connection using a client. This can be done using ssh on Linux/MacOS with the following command:

```
ssh -D 55051 user@server
```

On Windows, you can enable a SOCKS5 proxy by using PuTTY. More information on that can be found [here](https://www.simplified.guide/putty/create-socks-proxy).

Keep in mind that the port you open the connection with must match the one specified in the visus.config file.
