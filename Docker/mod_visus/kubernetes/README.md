# First Kubernet setup

Make sure completition is enabled:

```
source <(kubectl completion bash)
```

Create the Kubernet namespace you will use for resource creation:

```
kubectl create namespace openvisus
```

Later on you can remove all resources with a simple:

```
kubectl delete namespace openvisus
```

# Create Kubernet deployment

Create a `mod-visus-deployment` deployment for example using the following command:

```
kubectl create deployment --namespace openvisus  mod-visus-deployment --image=visus/mod_visus:latest  --dry-run -o yaml > mod_visus_deployment.yaml
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
kubectl describe deployment --namespace=openvisus mod-visus-deployment
kubectl get pods --namespace=openvisus
```

You can use `explain` keyword to check YAML syntax, for example:

```
kubectl explain Pod.spec.volumes
```

Or you can check the history of the deployment

```
kubectl rollout history --namespace openvisus deployment mod-visus-deployment
kubectl rollout status  --namespace openvisus deployment mod-visus-deployment
```

To get all the pods connected to the deployment:

```
kubectl get pods --namespace=openvisus  -o wide
```

To check if openvisus server are running you can create a temporary pod inside the cluster, :

```
kubectl run -i --tty --rm tmp1 --image=busybox --namespace=openvisus -- sh

# change the IP address with one given from get pods command one step above
IP_ADDRESS=10.1.0.34 

wget -O- --no-verbose http://$IP_ADDRESS/mod_visus?action=list 
exit
```

Or you can connect to one of the server pod:

```
kubectl exec --namespace=openvisus deploy/mod-visus-deployment -i -t -- sh
apt-get install -y wget
wget -O- --no-verbose http://localhost:80/mod_visus?action=list
exit
```

To inspect logs from the outside:

```
kubectl logs --namespace=openvisus deployment/mod-visus-deployment
```

# Create Kubernet Service

So far, no pods, nodes, deployment is already accessible from the outside.
To do so you  need to expose the deployment:

```
kubectl expose deployment --namespace openvisus mod-visus-deployment --port=80 --target-port=80 --name=mod-visus-service --type=NodePort --dry-run -o yaml > mod_visus_service.yaml
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
  namespace: openvisus # scrgiorgio: add the namespace
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
kubectl get service --namespace=openvisus
kubectl describe service --namespace=openvisus mod-visus-service
```

Check all resources in openvisus and all namespace:

```
kubectl get all --namespace=openvisus -o wide 
kubectl get all --all-namespaces -o wide 
```

You can access the service from a temporary pod using the service name:

```
kubectl run -i --tty --rm tmp1 --image=busybox --namespace=openvisus -- sh
wget -O- http://mod-visus-service/mod_visus?action=list
```

Or you can connect from external:

```
#if you don't know the nodePort (i.e. the port exposed to the external)
kubectl get service --namespace=openvisus -o yaml | grep nodePort

wget -O- --no-verbose http://localhost:30080/mod_visus?action=list
```

# (OPTIONAL; needed if you have a real clusted) Create ingress 

On Windows WSL2 (BROKEN right now, see: https://github.com/docker/for-win/issues/7094):

```
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-0.32.0/deploy/static/provider/cloud/deploy.yaml
https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-0.32.0/deploy/static/provider/cloud/deploy.yaml
```

On minikube:

```
minikube addons enable ingress
```


Create the file for ingress:

```
cat <<EOF  > mod_visus_ingress.yaml
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
EOF
```

Create/ Inspect/ Debug the ingress:

```
kubectl apply -f mod_visus_ingress.yaml
kubectl describe ingress --namespace=openvisus mod-visus-ingress 
kubectl get ingress --namespace=openvisus
```

See if it works:

```
wget -O- --no-verbose http://localhost:80/mod_visus?action=list
```


# (OPTIONAL). Add Network policy and ssh for better security

TODO
