#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
#define checkResults(string, val) {             \
 if (val) {                                     \
   printf("Failed with %d at %s", val, string); \
   exit(1);                                     \
 }                                              \
}
 
#define                 NUMTHREADS    10 
pthread_mutex_t         dataMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t          dataPresentCondition = PTHREAD_COND_INITIALIZER;
int                     dataPresent=0;
int                     sharedData=0;
 
void *theThread(void *parm)
{
   int   rc;
   int   retries=2;
 
   printf("Consumer Thread %.8x %.8x: Entered\n", pthread_self());
   rc = pthread_mutex_lock(&dataMutex);
   checkResults("pthread_mutex_lock()\n", rc);
 
   while (retries--) {
      /* The boolean dataPresent value is required for safe use of */
      /* condition variables. If no data is present we wait, other */
      /* wise we process immediately.                              */
      while (!dataPresent) {
         printf("Consumer Thread %.8x %.8x: Wait for data to be produced\n");
         rc = pthread_cond_wait(&dataPresentCondition, &dataMutex);
         if (rc) {
            printf("Consumer Thread %.8x %.8x: condwait failed, rc=%d\n",rc);
            pthread_mutex_unlock(&dataMutex);
            exit(1);
         }
      }
      printf("Consumer Thread %.8x %.8x: Found data or Notified, "
             "CONSUME IT while holding lock\n",
             pthread_self());
      /* Typically an application should remove the data from being    */
      /* in the shared structure or Queue, then unlock. Processing     */
      /* of the data does not necessarily require that the lock is held */
      /* Access to shared data goes here */
      --sharedData;
      /* We consumed the last of the data */
      if (sharedData==0) {dataPresent=0;}
      /* Repeat holding the lock. pthread_cond_wait releases it atomically */
   }
   printf("Consumer Thread %.8x %.8x: All done\n",pthread_self());
   rc = pthread_mutex_unlock(&dataMutex);
   checkResults("pthread_mutex_unlock()\n", rc);
   return NULL;
}
 
int main(int argc, char **argv)
{
   pthread_t             thread[NUMTHREADS];
   int                   rc=0;
   int                   amountOfData=1000;
   int                   i;
 
   printf("Enter Testcase - %s\n", argv[0]);
 
   printf("Create/start threads\n");
   for (i=0; i <NUMTHREADS; ++i) {
 rc = pthread_create(&thread[i], NULL, theThread, NULL);
      checkResults("pthread_create()\n", rc);
   }
 
   /* The producer loop */
   while (amountOfData--) {
      printf("Producer: 'Finding' data\n");
      sleep(3);
 
      rc = pthread_mutex_lock(&dataMutex);   /* Protect shared data and flag  */
      checkResults("pthread_mutex_lock()\n", rc);
      printf("Producer: Make data shared and notify consumer\n");
      ++sharedData;                          /* Add data                      */
      dataPresent=1;                         /* Set boolean predicate         */
 
      rc = pthread_cond_signal(&dataPresentCondition); /* wake up a consumer  */
      if (rc) {
         pthread_mutex_unlock(&dataMutex);
         printf("Producer: Failed to wake up consumer, rc=%d\n", rc);
         exit(1);
      }
 
      printf("Producer: Unlock shared data and flag\n");
      rc = pthread_mutex_unlock(&dataMutex);
      checkResults("pthread_mutex_lock()\n",rc);
   }
 
   printf("Wait for the threads to complete, and release their resources\n");
   for (i=0; i <NUMTHREADS; ++i) {
  rc = pthread_join(thread[i], NULL);
      checkResults("pthread_join()\n", rc);
   }
 
   printf("Clean up\n");
   rc = pthread_mutex_destroy(&dataMutex);
   rc = pthread_cond_destroy(&dataPresentCondition);
   printf("Main completed\n");
   return 0;
}

