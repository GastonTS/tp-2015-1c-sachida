#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>

#include "../src/mongo/mongo_file.h"
#include "../src/mongo/mongo_dir.h"
#include "bitarrayTest.h"
#include "../src/mongo/mongo_node.h"
#include "../src/console/console.h"
#include "../src/structs/node.h"
#include "../src/filesystem/filesystem.h"

int main(void) {

/*
	filesystem_initialize();


	file_t *file = file_create();
	strcpy(file->name, "archivo prueba");
	strcpy(file->parentId, ROOT_DIR_ID);
	file->size = 0;

	filesystem_copyFileFromFS("/home/pmgomez/a", file);

	file_free(file);
	*/
	
	node_t* node = node_create(100);

	node->name = strdup("NodeTest");

	node_printBlocksStatus(node);

	node_setBlockUsed(node, 1);
	node_setBlockUsed(node, 2);
	node_setBlockUsed(node, 23);
	node_setBlockUsed(node, 10);
	node_setBlockUsed(node, 15);
	node_setBlockUsed(node, 18);
	node_setBlockUsed(node, 6);
	node_setBlockUsed(node, 7);

	mongo_node_save(node);

	node_printBlocksStatus(node);

	node_free(node);


	node_t* node2 = mongo_node_getByName("NodeTest");

	node_printBlocksStatus(node2);
	//node_setBlockUsed(node2, 3);
	//mongo_node_updateBlocks(node2);
	node_free(node2);
	


	//filesystem_shutdown();
	return 1;
}