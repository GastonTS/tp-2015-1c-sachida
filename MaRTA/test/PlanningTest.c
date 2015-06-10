#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "../src/MaRTA.h"
#include "../src/structs/nodo.h"
#include "../src/structs/job.h"
#include "../src/Planning/MapPlanning.h"
#include "../src/Planning/ReducePlanning.h"

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

t_copy *unavailableCopy;
t_list *unavailableCopies;

t_file *file;
t_file *file2;
t_file *notAvailableFile;

t_job *job;
t_job *job2;

void setup() {
	node1 = malloc(sizeof(t_node));
	node1->active = 1;
	node1->ip = "IP Nodo1";
	node1->port = 3001;
	strcpy(node1->name, "Node1");
	node1->maps = list_create();
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	node1->reduces = list_create();
	list_add(node1->reduces, (void *) "datos.txt");

	node2 = malloc(sizeof(t_node));
	node2->active = 1;
	node2->ip = "IP Nodo2";
	node2->port = 3002;
	strcpy(node2->name, "Node2");
	node2->maps = list_create();
	list_add(node2->maps, (void *) 1);
	node2->reduces = list_create();
	list_add(node2->reduces, (void *) "datos.txt");

	node3 = malloc(sizeof(t_node));
	node3->active = 0;
	node3->ip = "IP Nodo3";
	node3->port = 3003;
	strcpy(node3->name, "Node3");
	node3->maps = list_create();
	node3->reduces = list_create();

	node4 = malloc(sizeof(t_node));
	node4->active = 1;
	node4->ip = "IP Nodo4";
	node4->port = 3004;
	strcpy(node4->name, "Node4");
	node4->maps = list_create();
	node4->reduces = list_create();

	node5 = malloc(sizeof(t_node));
	node5->active = 1;
	node5->ip = "IP Nodo5";
	node5->port = 3005;
	strcpy(node5->name, "Node5");
	node5->maps = list_create();
	node5->reduces = list_create();

	list_add(nodes, node1);
	list_add(nodes, node2);
	list_add(nodes, node3);
	list_add(nodes, node4);
	list_add(nodes, node5);

	copy1 = malloc(sizeof(t_copy));
	strcpy(copy1->nodeName, "Node3");
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

	file = malloc(sizeof(t_file));
	file->path = "sarasa.txt";
	file->blocks = list_create();
	list_add(file->blocks, copies1);
	list_add(file->blocks, copies2);

	file2 = malloc(sizeof(t_file));
	file2->path = "pepe.txt";
	file2->blocks = list_create();
	list_add(file2->blocks, copies3);

	job = malloc(sizeof(t_job));
	job->id = 42;
	job->combiner = false;
	job->files = list_create();
	list_add(job->files, file2);
	list_add(job->files, file);
	job->maps = list_create();

	unavailableCopy = malloc(sizeof(t_copy));
	strcpy(unavailableCopy->nodeName, "Node3");
	unavailableCopy->numBlock = 821;

	unavailableCopies = list_create();
	list_add(unavailableCopies, (void *) unavailableCopy);

	notAvailableFile = malloc(sizeof(t_file));
	notAvailableFile->path = "Inexistente.txt";
	notAvailableFile->blocks = list_create();
	list_add(notAvailableFile->blocks, unavailableCopies);

	job2 = malloc(sizeof(t_job));
	job2->id = 23;
	job2->files = list_create();
	list_add(job2->files, notAvailableFile);
	job2->maps = list_create();
}

void freeSetup() {
	list_destroy_and_destroy_elements(nodes, (void *) freeNode);
	freeJob(job);
	freeJob(job2);
}

void jobMapTest() {
	printf("**************************jobMapTest****************************\n");
	jobMap(job);
	jobMap(job2);
	printf("****************************************************************\n");
}

void RePlanTest() {
	printf("**************************RePlanTest****************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	node5->active = 0;
	printf("************************RePlanningMap(1)*************************\n");
	rePlanMap(job, 1);
	printf("****************************************************************\n");
}

void ReducePlanTest() {
	printf("************************ReducePlanTest**************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	printf("*************************reducePlanning**************************\n");
	reducePlanning(job);
	printf("****************************************************************\n");
}
