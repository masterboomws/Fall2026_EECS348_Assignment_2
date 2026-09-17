/* ============================================================  
* Main.c  
* C program that manages emails for a busy CEO using a max heap
* file passed as input
* Output via terminal
* No other Collaborators
* Sources: Claude
* Wyatt Sullivan
* Creation Date: 09/16/2026
* Modified: 09/17/2026
* ============================================================ */  

#define _POSIX_C_SOURCE 200809L 
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <ctype.h>  

/* ------------------------------------------------------------  
* Email record  
* ------------------------------------------------------------ */  

#define MAX_SENDER 64  //defines max length for the sender line
#define MAX_SUBJECT 256  //defines max length for the subject line
#define MAX_DATE 16  //defines max length for the date line

typedef struct { //defines the Email struct 
	char sender[MAX_SENDER]; /* the raw category string, e.g. "Boss" */  
	char subject[MAX_SUBJECT]; // The subject line as a string
	char date[MAX_DATE]; /* MM-DD-YYYY, stored as given */  
	int categoryRank; /* precomputed numeric priority for speed */  
	long dateKey; /* YYYYMMDD as an integer for comparisons */  
} Email;  

/* ------------------------------------------------------------  
* MaxHeap : list-based (dynamic array) implementation  
* ------------------------------------------------------------ */  
typedef struct {  //defines the maxHeap struct
	Email *data; /* backing array (the "list") */  
	int size; /* number of elements currently stored */  
	int capacity; /* allocated capacity of data[] */  
} MaxHeap;  

/* ---- forward declarations --------------------------------- */  
static void heapInit(MaxHeap *h, int initialCapacity);  
static void heapFree(MaxHeap *h);  
static void heapGrowIfNeeded(MaxHeap *h);  
static int emailIsHigherPriority(const Email *a, const Email *b);  
static void heapSwap(Email *a, Email *b);  
static void heapSiftUp(MaxHeap *h, int index);  
static void heapSiftDown(MaxHeap *h, int index);  
static void heapInsert(MaxHeap *h, Email e);  
static int heapExtractMax(MaxHeap *h, Email *out); /* returns 0/1 success */  
static int heapPeek(const MaxHeap *h, Email *out); /* returns 0/1 success */  
  
/* ------------------------------------------------------------  
* Category rank: higher number == higher priority  
* ------------------------------------------------------------ */  
static int categoryRank(const char *category) //function that returns the priority for a given sender 
{  

	if (strcmp(category, "Boss") == 0) return 5;  //Highest priority
	if (strcmp(category, "Subordinate") == 0) return 4;  //Second priority
	if (strcmp(category, "Peer") == 0) return 3;  //Third priority
	if (strcmp(category, "ImportantPerson") == 0) return 2;  //Fourth Priority
	if (strcmp(category, "OtherPerson") == 0) return 1;  //Last priority
	return 0; /* unknown category: lowest priority, shouldn't occur */  
}  

/* Convert "MM-DD-YYYY" into an integer YYYYMMDD so that a later  
* date compares as numerically larger than an earlier one. */  
static long dateToKey(const char *date)  //function that converts date to the correct format

{  
	int mm = 0, dd = 0, yyyy = 0; //declares variables to store date 
	if (sscanf(date, "%d-%d-%d", &mm, &dd, &yyyy) != 3) {  //checks if date is correct
		return 0; /* malformed date -> treat as earliest possible */  
	}  
	return ((long)yyyy * 10000L) + ((long)mm * 100L) + dd;  //returns date in correct format
}  

/* ============================================================  
* MaxHeap implementation  
* ============================================================ */  

static void heapInit(MaxHeap *h, int initialCapacity)  //function to initialise a max heap

{  
	if (initialCapacity < 4) initialCapacity = 4;  //corrects the initial capacity
	h->data = (Email *)malloc(sizeof(Email) * (size_t)initialCapacity);  //Sets the data of the maxheap to a list of  emails
	h->size = 0;  //sets the intial size of the max heap to 0
	h->capacity = initialCapacity;  //sets the initial capacity to initial capacity
}  

static void heapFree(MaxHeap *h)  

{  
	free(h->data); //calls free function with the data in the max heap 
	h->data = NULL;  //sets the data in the max heap to nothing
	h->size = 0;  //sets the size of the max heap to 0
	h->capacity = 0;  //sets the capacity of the max heap to 0
}  

static void heapGrowIfNeeded(MaxHeap *h)  

{  
	if (h->size >= h->capacity) {  //checks if the size is larger or equal to the capacity
		int newCapacity = h->capacity * 2;  //creates a new capacity that is twice the size of the previous
		Email *newData = (Email *)realloc(h->data, sizeof(Email) * (size_t)newCapacity);  //creates new data with the new capacity
		if (newData == NULL) {  //checks if the newData failed to create
			fprintf(stderr, "Error: out of memory while growing heap.\n");  //outputs error to user
			exit(EXIT_FAILURE);  //exits the program
		}  
		h->data = newData;  //sets the heap data to the new data
		h->capacity = newCapacity;  //sets the heap capacity to the new capacity
	}  
}  

/* Returns 1 if email a has strictly higher priority than email b,  
* 0 otherwise. This is the single source of truth for ordering. */  
static int emailIsHigherPriority(const Email *a, const Email *b)  

{  
	if (a->categoryRank != b->categoryRank) {  //checks if email a's category rank is not email b's rank
		return a->categoryRank > b->categoryRank;  //return
	}  
/* Same category: newer date (larger dateKey) is higher priority */  
	return a->dateKey > b->dateKey;  //return
}  

//Swaps the data of the two emails
static void heapSwap(Email *a, Email *b)  

{  
Email tmp = *a; //creates a temp email with data at a 
*a = *b;  //changes the data at a to the data at b
*b = tmp;  //changes the data at b to the temp data
}  

/* Move the element at index up until heap property is restored. */  
static void heapSiftUp(MaxHeap *h, int index)  

{  
	while (index > 0) {  //while the index is greater than 0
		int parent = (index - 1) / 2;  //sets the parent using the current index
		if (emailIsHigherPriority(&h->data[index], &h->data[parent])) {  //checks if the email at the current index is a higher priority than the parent
			heapSwap(&h->data[index], &h->data[parent]);  //swaps the emails
			index = parent;  //sets the next index to the parent
		} else {  
			break;  //exits loop
		}  
	}  
}  

/* Move the element at index down until heap property is restored. */  
static void heapSiftDown(MaxHeap *h, int index)  

{  
	while (1) {  //runs until exited
		int left = 2 * index + 1; //sets left index value relative to current index 
		int right = 2 * index + 2;  //sets right index value relative to current index
		int largest = index;  //the current larges is the current index
   		if (left < h->size && emailIsHigherPriority(&h->data[left], &h->data[largest])) { //checks if left is larger 
			largest = left;  //sets current largest to left index
		}  
		if (right < h->size && emailIsHigherPriority(&h->data[right], &h->data[largest])) {  //checks if right is larger
			largest = right; //sets current largest to right index
		}  
		if (largest == index) break; //if current index is largest break out of function 
		heapSwap(&h->data[index], &h->data[largest]);  //calls heap swap function
		index = largest;  //sets current index to the largest
	}  
}  

static void heapInsert(MaxHeap *h, Email e)  

{  
	heapGrowIfNeeded(h);  //grows the heap if needed
	h->data[h->size] = e;  //adds email e to the heap
	heapSiftUp(h, h->size);  //calls shift up function
	h->size++;  //increments the size up 1
}  

/* Removes and returns (via out) the highest-priority email.  
* Returns 1 on success, 0 if the heap was empty. */  
static int heapExtractMax(MaxHeap *h, Email *out)  

{  
	if (h->size == 0) return 0;  //if the size is empty returns 0
	if (out != NULL) {  //if the email is not null
		*out = h->data[0];  //sets out to the first email in the heap
	}  
	h->size--;  //decreases the size of the heap
	if (h->size > 0) {  //if the heap size is larger than 0
		h->data[0] = h->data[h->size];  //sets the first value in data to the lsat value in data
		heapSiftDown(h, 0); //runs heap shift down
	}  
	return 1;  //returns 1
}  

/* Looks at (without removing) the highest-priority email.  
* Returns 1 on success, 0 if the heap was empty. */  
static int heapPeek(const MaxHeap *h, Email *out)  

{  
	if (h->size == 0) return 0; //if the heap is empty return 0 
	if (out != NULL) {  //if out is not empty
		*out = h->data[0];  //set out to the first value in the heap data
	}  
	return 1;  //return 1
}  

/* ============================================================  
* Command parsing helpers  
* ============================================================ */  

/* Strip trailing '\n', '\r', and any trailing whitespace in-place. */  

static void trimTrailing(char *s)  

{  
	size_t len = strlen(s); //creates an unsigned integer with value strlen(s) 
	while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || isspace((unsigned char)s[len - 1]))) {  
		s[len - 1] = '\0';  
		len--;  
	}  
}  

   

/* Parse a line of the form:  
* EMAIL <category>,<subject>,<date>  
* (the line passed in has already had the leading "EMAIL " removed)  
* Fields are comma-delimited; subject may contain spaces but not commas.  
* Returns 1 on success, 0 on malformed input. */  

static int parseEmailFields(char *rest, Email *e)  

{  
/* rest looks like: "Peer,Can you help me on this?,12-01-2024" */  
	char *category = strtok(rest, ","); //category of the email input
	char *subject = strtok(NULL, ",");  //subject of the email inpus
	char *date = strtok(NULL, ",");  //date of the email input

	if (category == NULL || subject == NULL || date == NULL) { //if any of the parts are missing  
		return 0;  //return 0
	}  

	/* Category may have stray surrounding whitespace; trim it. */  

	while (isspace((unsigned char)*category)) category++;

	/* Subject: trim a single leading space if present (after the comma),  
	* but preserve internal spaces since subjects may contain them. */  

	if (*subject == ' ') subject++;  //trim the subject

	if (*date == ' ') date++; //trim the date 

	trimTrailing(date);  //trim the trailing spaces on the date

	strncpy(e->sender, category, MAX_SENDER - 1);  //copy the category to the email sender
	e->sender[MAX_SENDER - 1] = '\0'; //verify that the string is not too long

	strncpy(e->subject, subject, MAX_SUBJECT - 1);  //copy the subject to the email subject
	e->subject[MAX_SUBJECT - 1] = '\0';  //verify that the string is not too long

	strncpy(e->date, date, MAX_DATE - 1);  //copy the date to the email date
	e->date[MAX_DATE - 1] = '\0';  //verify that the string is not too long

	e->categoryRank = categoryRank(e->sender); //get the rank from the categoryRank function (1-5 based on sender)
	e->dateKey = dateToKey(e->date); //convert the date to the right format

	return 1;  //end function
}  

/* ============================================================  
* Main driver  
* ============================================================ */  

int main(void)  

{  
	MaxHeap heap;  //create the heap object
	heapInit(&heap, 16);  //initialise the heap
   
	char *line = NULL;  //create a char called line
	size_t lineCap = 0;  //size of line
	ssize_t nread;  //signed integer called nread
   
	while ((nread = getline(&line, &lineCap, stdin)) != -1) { //while there are more lines to be read
		(void)nread;
		trimTrailing(line); //trim the trailing spaces of line
		if (line[0] == '\0') {  //check if line is blank
			continue; /* skip blank lines */  
		}  
		if (strncmp(line, "EMAIL ", 6) == 0) {  //check if the first word on the line is email
			char *rest = line + 6;  //rest is the line after the 6th index
			Email e;  //create a new Email named e
			if (parseEmailFields(rest, &e)) {  //parse the rest of the email
				heapInsert(&heap, e);  //inserts in heap if the email had correct format
			} else {  
				fprintf(stderr, "Warning: malformed EMAIL line ignored: %s\n", line); //prints error if the email has incorrect format
			}  
		}  
		else if (strcmp(line, "NEXT") == 0) {  //checks if line is next
			Email top;  //creates email named top
			if (heapPeek(&heap, &top)) {  //checks what the top email is
				//block of code to output email to user
				printf("Next email:\n");  
				printf("\tSender: %s\n", top.sender);  
				printf("\tSubject: %s\n", top.subject);  
				printf("\tDate: %s\n", top.date);  
			} else {  
				printf("There are no emails to read.\n"); //message if there are no emails currently in the heap  
			}  
		}  
		else if (strcmp(line, "READ") == 0) {  
/* Removes the highest-priority email, if any, without  
* displaying anything -- matches spec for both the  
* normal case and two READs in a row with no NEXT. */  
			heapExtractMax(&heap, NULL);  
		}  
		else if (strcmp(line, "COUNT") == 0) {  //if the line is count
			printf("There are %d emails to read.\n", heap.size);  //outputs how many emails left to read to user
		}  
		else {  
			fprintf(stderr, "Warning: unrecognized command ignored: %s\n", line);  //error message that there input was invalid
		}  
	}  

	free(line);  //clears the line variable
	heapFree(&heap);  //clears the heap

	return 0;  //ends program
} 
