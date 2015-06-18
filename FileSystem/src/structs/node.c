#include <stdlib.h>
#include <string.h>

#include "node.h"

void node_free_blocks(node_t *node);
t_bitarray* getByteArrayForBlocksCount(uint16_t count);
bool node_blockIsValidIndex(node_t *node, off_t blockIndex);

bson_t* node_getBSON(node_t *node) {
	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "_id", node->id);
	BSON_APPEND_BINARY(bson, "blocks", BSON_SUBTYPE_BINARY, (uint8_t * ) node->blocks->bitarray, node->blocks->size);
	BSON_APPEND_INT32(bson, "blocksCount", node->blocksCount);
	return bson;
}

node_t* node_getNodeFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	node_t *node = malloc(sizeof(node_t));

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				node->id = strdup(value->value.v_utf8.str);
			} else if (strcmp(key, "blocks") == 0) {
				char *bitarray = malloc(value->value.v_binary.data_len);
				memcpy(bitarray, value->value.v_binary.data, value->value.v_binary.data_len);
				node->blocks = bitarray_create(bitarray, value->value.v_binary.data_len);
			} else if (strcmp(key, "blocksCount") == 0) {
				node->blocksCount = value->value.v_int32;
			}

		}
	}

	return node;
}

void node_setAllBlocksFree(node_t *node) {
	node_free_blocks(node);
	node->blocks = getByteArrayForBlocksCount(node->blocksCount);
}

void node_setBlockUsed(node_t *node, off_t blockIndex) {
	if (node_blockIsValidIndex(node, blockIndex)) {
		bitarray_set_bit(node->blocks, blockIndex);
	}
}

void node_setBlockFree(node_t *node, off_t blockIndex) {
	if (node_blockIsValidIndex(node, blockIndex)) {
		bitarray_clean_bit(node->blocks, blockIndex);
	}
}

bool node_isBlockUsed(node_t *node, off_t blockIndex) {
	if (node_blockIsValidIndex(node, blockIndex)) {
		return bitarray_test_bit(node->blocks, blockIndex);
	}
	return 0;
}

int node_getBlocksFreeCount(node_t *node) {
	int count = 0;
	int i;
	for (i = 0; i < node->blocksCount; i++) {
		if (!node_isBlockUsed(node, i)) {
			count++;
		}
	}
	return count;
}

off_t node_getFirstFreeBlock(node_t *node) {
	int i;
	for (i = 0; i < node->blocksCount; i++) {
		if (!node_isBlockUsed(node, i)) {
			return i;
		}
	}
	return -1;
}

node_t* node_create(uint16_t blocksCount) {
	node_t* node = malloc(sizeof(node_t));
	node->blocks = getByteArrayForBlocksCount(blocksCount);
	node->blocksCount = blocksCount;
	node->id = NULL;
	return node;
}

void node_free_blocks(node_t *node) {
	if (node->blocks) {
		if (node->blocks->bitarray) {
			free(node->blocks->bitarray);
		}
		bitarray_destroy(node->blocks);
	}
}

void node_free(node_t *node) {
	if (node) {
		if (node->id) {
			free(node->id);
		}
		node_free_blocks(node);
		free(node);
	}
}

t_bitarray* getByteArrayForBlocksCount(uint16_t count) {
	size_t size = (count + (CHAR_BIT - 1)) / CHAR_BIT; // Rounds up the division.
	char *data = malloc(size);

	int i;
	for (i = 0; i < size; i++) {
		data[i] = 0b00000000;
	}

	return bitarray_create(data, size);
}

bool node_blockIsValidIndex(node_t *node, off_t blockIndex) {
	return blockIndex < node->blocksCount;
}

void node_printBlocksStatus(node_t *node) {
	int i;
	printf("NODE %s BLOCKS USAGE: (left is 0-index) \n", node->id);

	for (i = 0; i < node->blocksCount; i++) {
		printf("%d", bitarray_test_bit(node->blocks, i));
		//printf("%d - %d \n", i, bitarray_test_bit(node->blocks, i));
	}
	printf("\n");
}
