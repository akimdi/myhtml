/*
 Copyright (C) 2016 Alexander Borisov
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 
 Authors: insoreiges@gmail.com (Evgeny Yakovlev)
 
 html2sexpr
 Convert html tag tree into s-expression string in stdout
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include <myhtml/api.h>

#define DIE(msg, ...) do { fprintf(stderr, msg, ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)

static bool filter_node(myhtml_tree_node_t* node) 
{
    assert(node);
    myhtml_tag_id_t tag = myhtml_node_tag_id(node);
    return (tag != MyHTML_TAG__TEXT) && (tag != MyHTML_TAG__END_OF_FILE) && (tag != MyHTML_TAG__COMMENT) && (tag != MyHTML_TAG__UNDEF);
}

/* depth-first lefthand tree walk */
static void walk_subtree(myhtml_tree_t* tree, myhtml_tree_node_t* root, int level)
{
    if (!root) {
        return;
    }

    /* Check if we handle this node type */
    if (!filter_node(root)) {
        return;
    }

    /* start sexpr */
    putchar('(');

    /* print this node */
    printf("%s", myhtml_tag_name_by_id(tree, myhtml_node_tag_id(root), NULL));
    myhtml_tree_attr_t* attr = myhtml_node_attribute_first(root);
    while (attr != NULL) {
        /* attribute sexpr (name value)*/
        const char *value = myhtml_attribute_value(attr, NULL);
        
        if (value)
            printf("(%s \'%s\')", myhtml_attribute_key(attr, NULL), value);
        else
            printf("(%s)", myhtml_attribute_key(attr, NULL));
        
        attr = myhtml_attribute_next(attr);
    }

    /* left hand depth-first recoursion */
    myhtml_tree_node_t* child = myhtml_node_child(root);
    while (child != NULL) {
        walk_subtree(tree, child, level + 1);
        child = myhtml_node_next(child);
    }

    /* close sexpr */
    putchar(')');
}

struct res_html {
    char  *html;
    size_t size;
};

struct res_html load_html_file(const char* filename)
{
    FILE *fh = fopen(filename, "rb");
    if(fh == NULL) {
        fprintf(stderr, "Can't open html file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    fseek(fh, 0L, SEEK_END);
    long size = ftell(fh);
    fseek(fh, 0L, SEEK_SET);
    
    char *html = (char*)malloc(size + 1);
    if(html == NULL) {
        DIE("Can't allocate mem for html file: %s\n", filename);
    }
    
    size_t nread = fread(html, 1, size, fh);
    if (nread != size) {
        DIE("could not read %ld bytes (%zu bytes done)\n", size, nread);
    }

    fclose(fh);
    
    if(size < 0) {
        size = 0;
    }
    
    struct res_html res = {html, (size_t)size};
    return res;
}

static void usage(void)
{
    fprintf(stderr, "html2sexpr <file>\n");
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage();
        DIE("Invalid number of arguments\n");   
    }

    struct res_html data = load_html_file(argv[1]);
    myhtml_status_t res;

	// basic init
    myhtml_t* myhtml = myhtml_create();
    res = myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
    if (MYHTML_FAILED(res)) {
    	DIE("myhtml_init failed with %d\n", res);
    }
    
    // init tree
    myhtml_tree_t* tree = myhtml_tree_create();
    res = myhtml_tree_init(tree, myhtml);
    if (MYHTML_FAILED(res)) {
    	DIE("myhtml_tree_init failed with %d\n", res);
    }
    
    // parse html
    myhtml_parse(tree, MyHTML_ENCODING_UTF_8, data.html, data.size);
    
    walk_subtree(tree, myhtml_tree_get_node_html(tree), 0);
    printf("\n");

    // release resources
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);
    free(data.html);

    return EXIT_SUCCESS;
}
