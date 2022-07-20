# configuration script for docs
# this script scrapes the existing OpenVisus repo (main branch) and adds it to the docs
# it also uses nbconvert to convert notebooks to markdown
# finally, it uses a markdown formatter to make things look clean

docs = []
notebooks = []