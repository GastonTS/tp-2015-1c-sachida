#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "../src/MaRTA.h"
#include "../src/structs/nodo.h"
#include "../src/structs/job.h"
#include "../src/Planning/MapPlanning.h"

t_node* node1;
t_node* node2;
t_node* node3;
t_node* node4;
t_node* node5;
t_copy *copy1;
t_copy *copy2;
t_copy *copy3;
t_list *copies1;
t_copy *copy2_1;
t_copy *copy2_2;
t_copy *copy2_3;
t_list *copies2;
t_copy *copy3_1;
t_copy *copy3_2;
t_copy *copy3_3;
t_copy *copy3_4;
t_copy *copy3_5;
t_list *copies3;
t_file *file;
t_file *file2;
t_job *job;

void setup(){
	node1 = malloc(sizeof(t_node));
		strcpy(node1->name, "Node1");
		node1->maps = list_create();
		list_add(node1->maps, (void *) 1);
		list_add(node1->maps, (void *) 1);
		list_add(node1->maps, (void *) 1);
		list_add(node1->maps, (void *) 1);
		node1->reduces = list_create();
		list_add(node1->reduces, (void *) "datos.txt");

	node2 = malloc(sizeof(t_node));
		strcpy(node2->name, "Node2");
		node2->maps = list_create();
		list_add(node2->maps, (void *) 1);
		node2->reduces = list_create();
		list_add(node2->reduces, (void *) "datos.txt");

	node3 = malloc(sizeof(t_node));
	strcpy(node3->name, "Node3");
	node3->maps = list_create();
	list_add(node3->maps, (void *) 1);
	node3->reduces = list_create();
	list_add(node3->reduces, (void *) "datos.txt");
	list_add(node3->reduces, (void *) "datos.txt");
	list_add(node3->reduces, (void *) "datos.txt");

	node4 = malloc(sizeof(t_node));
	strcpy(node4->name, "Node4");
	node4->maps = list_create();
	node4->reduces = list_create();

	node5 = malloc(sizeof(t_node));
	strcpy(node5->name, "Node5");
	node5->maps = list_create();
	node5->reduces = list_create();

	list_add(nodes, node1);
	list_add(nodes, node2);
	list_add(nodes, node3);
	list_add(nodes, node4);
	list_add(nodes, node5);

	copy1 = malloc(sizeof(t_copy));
	strcpy(copy1->nodeName, "Node1");
	copy1->numBlock = 2;

	copy2 = malloc(sizeof(t_copy));
	strcpy(copy2->nodeName, "Node2");
	copy2->numBlock = 919;

	copy3 = malloc(sizeof(t_copy));
	strcpy(copy3->nodeName, "Node5");
	copy3->numBlock = 227;

	copies1 = list_create();
	list_add(copies1, (void *) copy1);
	list_add(copies1, (void *) copy2);
	list_add(copies1, (void *) copy3);

	copy2_1 = malloc(sizeof(t_copy));
	strcpy(copy2_1->nodeName, "Node3");
	copy2_1->numBlock = 7;

	copy2_2 = malloc(sizeof(t_copy));
	strcpy(copy2_2->nodeName, "Node5");
	copy2_2->numBlock = 307;

	copy2_3 = malloc(sizeof(t_copy));
	strcpy(copy2_3->nodeName, "Node4");
	copy2_3->numBlock = 13;

	copies2 = list_create();
	list_add(copies2, (void *) copy2_1);
	list_add(copies2, (void *) copy2_2);
	list_add(copies2, (void *) copy2_3);

	copy3_1 = malloc(sizeof(t_copy));
	strcpy(copy3_1->nodeName, "Node1");
	copy3_1->numBlock = 17;

	copy3_2 = malloc(sizeof(t_copy));
	strcpy(copy3_2->nodeName, "Node3");
	copy3_2->numBlock = 421;

	copy3_3 = malloc(sizeof(t_copy));
	strcpy(copy3_3->nodeName, "Node5");
	copy3_3->numBlock = 23;

	copy3_4 = malloc(sizeof(t_copy));
	strcpy(copy3_4->nodeName, "Node2");
	copy3_4->numBlock = 29;

	copy3_5 = malloc(sizeof(t_copy));
	strcpy(copy3_5->nodeName, "Node4");
	copy3_5->numBlock = 821;

	copies3 = list_create();
	list_add(copies3, (void *) copy3_1);
	list_add(copies3, (void *) copy3_2);
	list_add(copies3, (void *) copy3_3);
	list_add(copies3, (void *) copy3_4);
	list_add(copies3, (void *) copy3_5);

	file=malloc(sizeof(t_file));
	file->path="sarasa.txt";
	file->blocks=list_create();
	list_add(file->blocks, copies1);
	list_add(file->blocks, copies2);

	file2=malloc(sizeof(t_file));
	file2->path="sarasa.txt";
	file2->blocks=list_create();
	list_add(file->blocks, copies3);

	job = malloc(sizeof(t_job));
	job->id = 42;
	job->files = list_create();
	list_add(job->files, file);
	list_add(job->files, file2);
	job->maps = list_create();
}

void freeSetup(){
	list_destroy_and_destroy_elements(nodes, (void *) freeNode);
	freeJob(job);
}

void jobMapTest(){
	printf("**************************jobMapTest****************************\n");
	jobMap(job);
	printf("****************************************************************\n");
}
