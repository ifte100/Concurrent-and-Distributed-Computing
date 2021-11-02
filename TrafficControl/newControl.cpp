#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>

using namespace std;
//using std::ifstream;

#define NUM_PRODUCERS 8
#define NUM_CONSUMERS 2
#define TRAFFIC_LIGHTS 20

const int SIZE = 10;

//says we need to find N lights so upto us but need to take 12 measurements
int light_measurement = TRAFFIC_LIGHTS * 12;

//custom data structure to simulate traffic
struct Traffic 
{
    time_t time;
    unsigned traffic_light_id;
    unsigned no_of_cars;
};

//counting traffic
struct Count
{
    int id;
    int count;
};

//shows the current index of the element in the buffer
int idx = 0;

//initially empty 
Traffic data[SIZE] = {};

Count traffic_count[TRAFFIC_LIGHTS];

//these are condition variables used for locking mechanism
//we can define a lock->two conditions->signal threads and awake all threads
//all consumers sleep until some data is there
//as soon as one producer produce, broadcast to sleeping consumers
//cannot produce if buffer is full, and cannot consume if there is not atleast one in the buffer
pthread_cond_t canProduce;
pthread_cond_t canConsume;

pthread_mutex_t lock;

ifstream file("randomData.csv");

void *produce(void *args)
{
    //file.open("data.csv");
    string line = "";
    char delimeter = ',';
    int current = 0;

    while(file)
    {
        pthread_mutex_lock(&lock);
            if(file.is_open())
            {
                getline(file, line);
            }
        pthread_mutex_unlock(&lock);
        //if not blank line
        if (line != "")
        {
            struct Traffic *traffic = (struct Traffic*) malloc(sizeof(struct Traffic));

            current = line.find(delimeter);

            traffic -> time = (time_t)stoi(line.substr(0, current));

            line.erase(0, current + 1);

            current = line.find(delimeter);

            traffic -> traffic_light_id = stoi(line.substr(0, current));

            line.erase(0, current + 1);
            //last one has no delimeter so dont put delimeter
            traffic -> no_of_cars = stoi(line);

            pthread_mutex_lock(&lock);
            while(idx >= SIZE)            //if there is no more size in the buffer just wait
            {
                pthread_cond_wait(&canProduce, &lock);
            }

            //data[idx] = pthread_self();
            data[idx] = *traffic;
            idx++;
            pthread_cond_broadcast(&canConsume);
            pthread_mutex_unlock(&lock);
        }

    }
    return NULL;
}

bool sorting_function(Count a, Count b)
{
    return a.count > b.count;
}

int current = 0;

void *consume(void *args)
{

    while (true)
    {
        pthread_mutex_lock(&lock);
        while (idx == 0)
        {
            pthread_cond_wait(&canConsume, &lock);
        }
        Traffic traffic;
        for (size_t i = 0; i < idx; i++)
        {
            traffic = data[i];
            if (current < light_measurement)
            {
                traffic_count[traffic.traffic_light_id].count += traffic.no_of_cars;
                current++;
            }
            if (current == light_measurement)
            {
                sort(std::begin(traffic_count), std::end(traffic_count), sorting_function);
                cout << "The most congested traffic is at: \n";
                for (int i = 0; i < 5; i++)
                {
                    cout << "Traffic Light ID: " << traffic_count[i].id << " cars drove by " << traffic_count[i].count << "\n";
                }
                for (int i = 0; i < TRAFFIC_LIGHTS; i++)
                {
                    traffic_count[i].id = i;
                    traffic_count[i].count = 0;
                }
                current = 0;
                break;
            }
        }
        idx = 0;
        pthread_cond_broadcast(&canProduce);
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main()
{
    pthread_cond_init(&canProduce, NULL);
    pthread_cond_init(&canConsume, NULL);
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < TRAFFIC_LIGHTS; i++)
    {
       traffic_count[i].id = i;
       traffic_count[i].count = 0;
    }

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];

    for (size_t i = 0; i <NUM_PRODUCERS; i++)
    {
        pthread_create(&producers[0], NULL, produce, NULL);
    }

    for (size_t i = 0; i < NUM_CONSUMERS; i++)
    {
        pthread_create(&consumers[0], NULL, consume, NULL);
    }

    for (size_t i = 0; i < NUM_PRODUCERS; i++)
    {
        pthread_join(producers[i], NULL);
    }

    for (size_t i = 0; i < NUM_CONSUMERS; i++)
    {
        pthread_join(consumers[i], NULL);
    }
}