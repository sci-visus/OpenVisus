"""
 configuration script for OpenViSUS docs
 this script scrapes the existing OpenVisus repo (main branch) and adds it to the docs
 it also uses nbconvert to convert notebooks to markdown
 finally, it uses a markdown formatter to make things look clean
 you must run this script within this repository for it to scrape and modify files correctly
"""

from urllib.request import urlopen
import glob
import os
import shutil
import subprocess
import tempfile


def create_header(parent, title):
    HEADER = '---\nlayout: default\ntitle: {TITLE}\nparent: {PARENT}\nnav_order: 2\n---\n\n# Table of contents\n{: .no_toc .text-delta }\n\n1. TOC\n{:toc}\n\n---\n\n'
    return HEADER.replace('{TITLE}', title).replace('{PARENT}', parent)


def create_title(md_filename):
    return md_filename.replace(
        ".md",
        "").replace(
        ".",
        " ").replace(
            "-",
            " ").replace(
                "_",
        " ").title()


def save_markdown(parent_name, md_filename):
    with open(md_filename, 'r+') as file:
        content = file.read()
        file.seek(0)
        file.write(
            create_header(
                parent_name,
                create_title(md_filename)) +
            content)


def convert_old_docs(docs_base_url, docs, tmp_dir, docs_dir):
    for doc in docs:
        url = docs_base_url + doc
        response = urlopen(url).read().decode()
        with open(doc, 'w') as file:
            file.write(response)
            file.close()

    # add jekyll tags to all markdowns
    markdowns = glob.glob("*.md")
    for md in markdowns:
        save_markdown('Old Docs', md)

    # copy md files to proper docs directory
    shutil.copytree(
        os.getcwd(),
        os.path.join(
            docs_dir,
            'docs',
            'old-docs'),
        dirs_exist_ok=True)


def convert_jupyter_notebooks(
        notebooks_base_url,
        notebooks,
        tmp_dir,
        docs_dir):
    # download notebooks and convert to markdown
    for nb in notebooks:
        url = notebooks_base_url + nb
        response = urlopen(url).read().decode()
        with open(nb, 'w') as file:
            file.write(response)
            file.close()
        subprocess.run(
            "jupyter nbconvert --to markdown {}".format(nb),
            shell=True)
        os.remove(nb)

    # add jekyll tags to all markdowns
    markdowns = glob.glob("*.md")
    for md in markdowns:
        save_markdown('Jupyter Notebook Examples', md)

    # copy md files to proper docs directory
    shutil.copytree(
        os.getcwd(),
        os.path.join(
            docs_dir,
            'docs',
            'jupyter-examples'),
        dirs_exist_ok=True)


def main(tmp_dir, docs_dir):
    print("Using tmp directory: ", tmp_dir)

    # do all the old docs copying and conversion
    os.mkdir('old-docs')
    os.chdir(os.path.join(tmp_dir, 'old-docs'))
    docs_base_url = "https://raw.githubusercontent.com/sci-visus/OpenVisus/master/docs/"
    docs = [
        "atlantis.sci.utah.edu.md",
        "compilation.centos.md",
        "compilation.md",
        "conda_installation.md",
        "copy_datasets_to_s3.md",
        "docker_modvisus.md",
        "docker_swarm_modvisus.md",
        "kubernetes.md"]
    convert_old_docs(docs_base_url, docs, tmp_dir, docs_dir)

    os.chdir(tmp_dir)

    # do all the jupyter notebook copying and conversion
    os.mkdir('jupyter-examples')
    os.chdir(os.path.join(tmp_dir, 'jupyter-examples'))
    notebooks_base_url = "https://raw.githubusercontent.com/sci-visus/OpenVisus/master/Samples/jupyter/"
    notebooks = [
        "Agricolture.ipynb",
        "Climate.ipynb",
        "nasa_conversion_example.ipynb",
        "slice_query.ipynb"]
    convert_jupyter_notebooks(notebooks_base_url, notebooks, tmp_dir, docs_dir)


if __name__ == '__main__':
    THIS_DIR = os.getcwd()
    with tempfile.TemporaryDirectory() as TMP_DIR:
        os.chdir(TMP_DIR)
        main(TMP_DIR, THIS_DIR)
