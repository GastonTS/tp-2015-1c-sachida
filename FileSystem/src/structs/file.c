#include <stdlib.h>
#include <string.h>

#include "file.h"

void file_block_free(file_block_t *fileBlock);
file_block_t* file_block_getFileFromBSON(bson_t *doc);

bson_t* file_block_getBSON(file_block_t *fileBlock) {
	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "nodeId", fileBlock->nodeId);
	BSON_APPEND_INT32(bson, "blockIndex", fileBlock->blockIndex);
	return bson;
}

bson_t* file_getBSON(file_t *file) {
	char arrayKey[16];

	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "_id", file->id);
	BSON_APPEND_UTF8(bson, "name", file->name);
	BSON_APPEND_INT64(bson, "size", file->size);
	BSON_APPEND_UTF8(bson, "parentId", file->parentId);

	bson_t *bson_blocks = bson_new();
	int blockIndex = 0;
	void listBlocks(t_list* blockCopies) {
		bson_t *bson_block_copies = bson_new();
		int copyIndex = 0;
		void listBlockCopy(file_block_t *blockCopy) {
			sprintf(arrayKey, "%d", copyIndex);
			bson_t *bson_block_copy = file_block_getBSON(blockCopy);
			BSON_APPEND_DOCUMENT(bson_block_copies, arrayKey, bson_block_copy);
			bson_destroy(bson_block_copy);
			copyIndex++;
		}
		list_iterate(blockCopies, (void *) listBlockCopy);

		sprintf(arrayKey, "%d", blockIndex);
		BSON_APPEND_ARRAY(bson_blocks, arrayKey, bson_block_copies);
		bson_destroy(bson_block_copies);
		blockIndex++;
	}
	list_iterate(file->blocks, (void *) listBlocks);
	BSON_APPEND_ARRAY(bson, "blocks", bson_blocks);
	bson_destroy(bson_blocks);
	return bson;
}

file_t* file_getFileFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	file_t *file = file_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				strcpy(file->id, value->value.v_utf8.str);
			} else if (strcmp(key, "name") == 0) {
				strcpy(file->name, value->value.v_utf8.str);
			} else if (strcmp(key, "size") == 0) {
				file->size = value->value.v_int64;
			} else if (strcmp(key, "parentId") == 0) {
				strcpy(file->parentId, value->value.v_utf8.str);
			} else if (strcmp(key, "blocks") == 0) {
				bson_t array;
				bson_iter_t array_iter;
				bson_init_static(&array, value->value.v_doc.data, value->value.v_doc.data_len);

				if (bson_iter_init(&array_iter, &array)) {
					while (bson_iter_next(&array_iter)) {
						value = bson_iter_value(&array_iter);

						bson_t array;
						bson_iter_t array_iter;
						bson_init_static(&array, value->value.v_doc.data, value->value.v_doc.data_len);

						if (bson_iter_init(&array_iter, &array)) {
							t_list *blockCopies = list_create();
							list_add(file->blocks, blockCopies);

							while (bson_iter_next(&array_iter)) {
								value = bson_iter_value(&array_iter);

								bson_t doc;
								bson_init_static(&doc, value->value.v_doc.data, value->value.v_doc.data_len);
								list_add(blockCopies, file_block_getFileFromBSON(&doc));
							}
						}
					}
				}
			}
		}
	}

	return file;
}

file_block_t* file_block_getFileFromBSON(bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	file_block_t *fileBlock = file_block_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "nodeId") == 0) {
				strcpy(fileBlock->nodeId, value->value.v_utf8.str);
			} else if (strcmp(key, "blockIndex") == 0) {
				fileBlock->blockIndex = value->value.v_int32;
			}
		}
	}

	return fileBlock;
}
file_t* file_create() {
	file_t* file = malloc(sizeof(file_t));
	file->name = malloc(sizeof(char) * 512);
	file->parentId = malloc(ID_SIZE);
	file->blocks = list_create();
	return file;
}

void file_free(file_t *file) {
	void freeBlocks(t_list *copies) {
		list_destroy_and_destroy_elements(copies, (void *) file_block_free);
	}
	list_destroy_and_destroy_elements(file->blocks, (void *) freeBlocks);
	free(file->name);
	free(file->parentId);
	free(file);
}

file_block_t* file_block_create() {
	file_block_t* fileBlock = malloc(sizeof(file_block_t));
	return fileBlock;
}

void file_block_free(file_block_t *fileBlock) {
	free(fileBlock);
}
