"""
The goal of this script is to generate empty markdown files with OpenVisus
functions headers for the purpose of documentation and annotation
"""

import ast
from tkinter.filedialog import Open
import OpenVisus # must have package installed to inspect functions
import os
from inspect import getargspec, getmembers, getmodulename, getsource, isfunction, signature

def skip(func): # filter functions so we document only needed user-facing functions
    func_source = getsource(func)
    uses_kernel = "_VisusKernelPy" in str(func_source)
    print(uses_kernel)
    has_return = any(isinstance(node, ast.Return) for node in ast.walk(ast.parse(func_source)))
    return not has_return or uses_kernel

def create_header(func_name, func_source):
    HEADER = '---\nlayout: default\ntitle: {}\nparent: All Functions\ngrand_parent: Python Functions\nnav_order: 2\n---\n\n# {}\n\nFunction description goes here.\n\n# Function Definition\n\n```python\n{}```'
    return HEADER.format(func_name, func_name, func_source)

def spawn_docs(module, pre=""):
    # functions_dict = dict([o for o in inspect.getmembers(module) if inspect.isclass(o[1])]) find all classes
    functions_dict = dict([o for o in getmembers(module) if isfunction(o[1])])
    for func_name in functions_dict:
        title = pre + func_name # for adjustments in naming scheme
        func = functions_dict[func_name]
        md_filename = "{}.md".format(title)
        #if os.path.exists(md_filename): continue # don't overrite existing files that already are documented
        content = create_header(title, getsource(func))
        with open(md_filename, 'w') as file:
            file.write(content)


def main():
    spawn_docs(OpenVisus, pre="OpenVisus.")
    spawn_docs(OpenVisus.dataset.PyDataset, pre="OpenVisus.dataset.PyDataset.")
        

if __name__ == '__main__':
    THIS_DIR = os.getcwd()
    TARGET = os.path.join(THIS_DIR, 'docs', 'python-functions', 'all')
    os.chdir(TARGET)
    main()
