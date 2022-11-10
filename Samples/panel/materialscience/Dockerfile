FROM ubuntu:22.04
RUN apt update -y 
RUN apt install -y wget unzip patchelf python3 python3-pip 
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install wheel numpy panda matplotlib jupyter ipywidgets bokeh ipywidgets-bokeh panel ipympl pandas OpenVisusNoGui
RUN python3 -m OpenVisus configure

