---
layout: default
title: Kubernetes
parent: Running the ViSUS Server
nav_order: 2
---

# Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

# Introduction

This tutorial shows how to run one or multiple modvisus with a frontend load balancer using Kubernetes (K8s).

See [docs/powerpoint/Kubernetes.pptx](https://github.com/sci-visus/OpenVisus/blob/master/docs/powerpoint/Kubernetes.pptx) for a graphical expolanation.

# Prerequisites

If you are running on Windows, and would like to experiment with K8s, please follow https://learnk8s.io/installing-docker-kubernetes-windows; you should install [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10), [Docker Desktop](https://www.docker.com/products/docker-desktop) and [minikube](https://minikube.sigs.k8s.io/docs/start/):

```
choco install minikube -y
minikube start
```


Make sure completition is enabled:

```
source <(kubectl completion bash)
```

And create a unique K8s namespace you will use for resource creation:

```
kubectl create namespace my-namespace
kubectl config set-context --current --namespace=my-namespace
```

Later on you can remove all resources with a simple by:

```
kubectl delete namespace my-namespace
```


# Create K8s deployment

Create a `mod-visus-deployment` deployment for example using the following command:

```
kubectl create deployment mod-visus-deployment --image=visus/mod_visus:latest  --dry-run -o yaml > mod_visus_deployment.yaml
```

Edit the file and setup as needed (`vi ./mod_visus_deployment.yaml`)

``` 
apiVersion: apps/v1
kind: Deployment
metadata:
  creationTimestamp: null
  labels:
    app: mod_visus_deployment
  name: mod-visus-deployment
  namespace: openvisus
spec:
  replicas: 3 # scrgiorgio: it was 1, change have N instances with load balancing
  selector:
    matchLabels:
      app: mod_visus_deployment
  template:
    metadata:
      creationTimestamp: null
      labels:
        app: mod_visus_deployment
    spec:
      # scrgiorgio: custom section for mounting nfs path in case you have some datasets to expose
      volumes:
      - name: datasets-volume
        hostPath:
          path: /mnt/c/projects/OpenVisus/datasets
      containers:
      - image: visus/mod_visus
        name: modvisus # scrgiorgio: change from mod_visus to modvisus , '_' character is not allowed
        
        # scrgiorgio:  custom section to configure datasets and ports 
        ports:                   
        - containerPort : 80   
        volumeMounts:
        - name: datasets-volume
          mountPath: /datasets # scrgiorgio: this is where mod_visus wants the datasets 
        env:                    
          - name: VISUS_DATASETS 
            value: /datasets  
        readinessProbe:
          httpGet:
            path: /mod_visus?action=list
            port: 80
        livenessProbe:
          httpGet:
            path: /
            port: 80
```


Finally create and inspect the deployment:

```
kubectl apply -f mod_visus_deployment.yaml  --record 
kubectl describe deployment mod-visus-deployment
kubectl get mod-visus-deployment -o wide
kubectl get pods -o wide
kubectl logs mod-visus-deployment
kubectl logs mod-visus-deployment-67d987676d-2vsvd # replace with the name of the logs
```



In case you don't rememeber specific YAML sections of your file, you can use `explain`. For example:

```
kubectl explain Pod.spec.volumes
```

Or you can check the history of the deployment

```
kubectl rollout history deployment mod-visus-deployment
kubectl rollout status  deployment mod-visus-deployment
```

To get all the pods connected to the deployment:

```
kubectl get pods -o wide
```

To check if openvisus server are running you can create a temporary pod inside the cluster:

```
kubectl run -i --tty --rm tmp1 --image=busybox  -- sh

# change the IP address with one given from `kubectl get pods -o wide` command:
IP_ADDRESS=10.1.0.34 

wget -O- --no-verbose http://$IP_ADDRESS/mod_visus?action=list 
exit
```

Or you can connect to one of the server pod:

```
kubectl exec  deploy/mod-visus-deployment -i -t -- sh
apt-get install -y wget
wget -O- --no-verbose http://localhost:80/mod_visus?action=list
exit
```

To inspect logs from the outside:

```
kubectl logs  deployment/mod-visus-deployment
```

# Create K8s Service

So far, no pods, nodes, deployment is already accessible from the outside.
To do so you  need to expose the deployment:

```
kubectl expose deployment mod-visus-deployment --port=80 --target-port=80 --name=mod-visus-service --type=NodePort --dry-run -o yaml > mod_visus_service.yaml
```

The edit the yaml file (`vi mod_visus_service.yaml`)

```
apiVersion: v1
kind: Service
metadata:
  creationTimestamp: null
  labels:
    app: mod-visus-deployment
  name: mod-visus-service
  namespace: openvisus # scrgiorgio: important to add
spec:
  ports:
  - port: 80
    protocol: TCP
    targetPort: 80
    nodePort: 30080 # scrgiorgio: set a static nodePort just for simplicity 
  selector:
    app: mod-visus-deployment
  type: NodePort
```

Then create the service using that file:

```
kubectl apply -f mod_visus_service.yaml --record
```

Inspect the service:

```
kubectl get service 
kubectl describe service mod-visus-service
```

Check all resources in openvisus:

```
kubectl get all  -o wide 
kubectl get all --all-namespaces -o wide 
```

You can access the service from a temporary pod using the service name:

```
kubectl run -i --tty --rm tmp1 --image=busybox -- sh
wget -O- http://mod-visus-service/mod_visus?action=list
```

Or you can connect from external:

```
#if you don't know the nodePort (i.e. the port exposed to the external)
kubectl get service -o yaml | grep nodePort

wget -O- --no-verbose http://localhost:30080/mod_visus?action=list
```

#  Create ingress (OPTIONAL needed only for real cluster)

On minikube:

```
minikube addons enable ingress
```


Create a ` mod_visus_ingress.yaml` file for ingress:

```
apiVersion: networking.k8s.io/v1beta1 # for versions before >1.16 use extensions/v1
kind: Ingress
metadata:
  name: mod-visus-ingress
  namespace: openvisus
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  rules:
  - host: localhost
    http:
      paths:
      - path: /
        backend:
          serviceName: mod-visus-service
          servicePort: 80 # note: this is the service port, not the nodePort (you could use even a Cluster service instead of NodePort service?)
```

Create/ Inspect/ Debug the ingress:

```
kubectl apply -f mod_visus_ingress.yaml
kubectl describe ingress mod-visus-ingress 
kubectl get ingress 
```

See if it works:

```
wget -O- --no-verbose http://localhost:80/mod_visus?action=list
```



bectl get ingress 
```

See if it works:

```
wget -O- --no-verbose http://localhost:80/mod_visus?action=list
```



