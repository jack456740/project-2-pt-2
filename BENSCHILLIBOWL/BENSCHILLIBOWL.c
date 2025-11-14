#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "BENSCHILLIBOWL.h"

// Feel free to play with these numbers! This is a great way to
// test your implementation.
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS * ORDERS_PER_CUSTOMER

// Global variable for the restaurant.
BENSCHILLIBOWL *bcb;

/**
 * Thread funtion that represents a customer. A customer should:
 *  - allocate space (memory) for an order.
 *  - select a menu item.
 *  - populate the order with their menu item and their customer ID.
 *  - add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;
    
    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        // Allocate space for an order
        Order* order = (Order*) malloc(sizeof(Order));
        if (!order) {
            printf("Customer %d: Failed to allocate memory for order\n", customer_id);
            continue;
        }
        
        // Select a menu item and populate the order
        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->next = NULL;
        
        // Add order to the restaurant
        int order_number = AddOrder(bcb, order);
        
        printf("Customer %d placed order #%d: %s\n", 
               customer_id, order_number, order->menu_item);
        
        // Small delay between orders from the same customer
        usleep(10000);
    }
    
    return NULL;
}

/**
 * Thread function that represents a cook in the restaurant. A cook should:
 *  - get an order from the restaurant.
 *  - if the order is valid, it should fulfill the order, and then
 *    free the space taken by the order.
 * The cook should take orders from the restaurants until it does not
 * receive an order.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;
    
    while (1) {
        // Get an order from the restaurant
        Order* order = GetOrder(bcb);
        
        // If no valid order received, break out of the loop
        if (order == NULL) {
            break;
        }
        
        // Fulfill the order (simulate cooking time)
        printf("Cook #%d is preparing order #%d for customer %d: %s\n", 
               cook_id, order->order_number, order->customer_id, order->menu_item);
        usleep(50000); // Simulate cooking time (50ms)
        
        // Free the memory taken by the order
        free(order);
        orders_fulfilled++;
        
        printf("Cook #%d completed order #%d\n", cook_id, order->order_number);
    }
    
    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Runs when the program begins executing. This program should:
 *  - open the restaurant
 *  - create customers and cooks
 *  - wait for all customers and cooks to be done
 *  - close the restaurant.
 */
int main() {
    // Seed random number generator
    srand(time(NULL));
    
    // Open the restaurant
    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);
    if (!bcb) {
        printf("Failed to open restaurant\n");
        return 1;
    }
    
    // Create arrays for customer and cook threads
    pthread_t customer_threads[NUM_CUSTOMERS];
    pthread_t cook_threads[NUM_COOKS];
    
    printf("Creating %d customers and %d cooks...\n", NUM_CUSTOMERS, NUM_COOKS);
    
    // Create cook threads first so they're ready to process orders
    for (int i = 0; i < NUM_COOKS; i++) {
        if (pthread_create(&cook_threads[i], NULL, BENSCHILLIBOWLCook, (void*)(long)(i + 1)) != 0) {
            printf("Failed to create cook thread %d\n", i);
            return 1;
        }
    }
    
    // Create customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if (pthread_create(&customer_threads[i], NULL, BENSCHILLIBOWLCustomer, (void*)(long)(i + 1)) != 0) {
            printf("Failed to create customer thread %d\n", i);
            return 1;
        }
        
        // Small delay between customer thread creation to simulate staggered arrivals
        usleep(5000);
    }
    
    printf("All threads created. Waiting for completion...\n");
    
    // Wait for all customer threads to complete
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customer_threads[i], NULL);
    }
    
    printf("All customers have placed their orders.\n");
    
    // Wait for all cook threads to complete
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cook_threads[i], NULL);
    }
    
    printf("All cooks have finished their work.\n");
    
    // Close the restaurant
    CloseRestaurant(bcb);
    
    printf("Restaurant simulation completed successfully!\n");
    printf("Expected orders: %d\n", EXPECTED_NUM_ORDERS);
    
    return 0;
}