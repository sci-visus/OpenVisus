/*
 * Copyright (c) 2017 University of Utah
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <libxml/xmlreader.h>
#include <libxml/xinclude.h>

int validate_xmf(char *filename, bool keep_validate_file) {
    xmlTextReaderPtr reader;
    int ret;
     
    xmlDocPtr doc = xmlReadFile(filename, NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT 
        | XML_PARSE_XINCLUDE );//| XML_PARSE_NOERROR);
    /*
     * apply the XInclude process, this should trigger the I/O just
     * registered.
     */
    bool includes = false;
    if (xmlXIncludeProcess(doc) <= 0) {
        fprintf(stderr, "XInclude processing failed. Are there any XInclude?\n");
        includes = true;
    }

    const char* tmp_file = "to_validate.xmf";
    xmlSaveFile(tmp_file, doc);

    /*
     * Free the document
     */
    xmlFreeDoc(doc);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();

    reader = xmlReaderForFile(tmp_file, NULL,
             XML_PARSE_DTDATTR | XML_PARSE_NOENT |
            XML_PARSE_DTDVALID | XML_PARSE_XINCLUDE); 

    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
    
        /*
         * Once the document has been fully parsed check the validation results
         */
        if (xmlTextReaderIsValid(reader) != 1) {
            fprintf(stderr, "Document %s is not valid\n", filename);
            return 1;
        }
        
    } else {
        fprintf(stderr, "Unable to open %s\n", filename);
        return 1;
    }
    
    printf("Validation done\n");

    if(!keep_validate_file)
        remove(tmp_file);

    return 0;
}

int main(int argc, char** argv){

    if(argc < 2){
        fprintf(stderr, "Usage: validate file_path [keep_processed_file]\n");

        return 1;
    }
    
    return validate_xmf(argv[1], argc > 2);
}
