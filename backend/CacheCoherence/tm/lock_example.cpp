#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

class Acct {
    int balance;
    uint64_t addr;
    public:
        pthread_mutex_t lock;
        int get();
        void put(int);
        Acct(uint64_t address);
};
class Bank{
    public:
    int num_accounts;
    Acct *array_acct;
    Bank(int num_accounts, uint64_t *addr);

};
Bank::Bank(int num_acct, uint64_t *addr){
    num_accounts = num_acct;
    array_acct = (Acct *)(malloc(sizeof(Acct)*num_acct));
    for(int i = 0; i < num_acct; i ++){
        array_acct[i] = *(new Acct(addr[i]));
    }
}
Acct::Acct(uint64_t address){
    balance = 1000; //don't handle case of negative balance
    addr = address;
    if(pthread_mutex_init(&lock,NULL)) printf("init error\n");
}
int Acct::get(){
    return balance;
}

void Acct::put(int amount){
    balance = amount;
}

int deposit(Acct *account, int amount){
    pthread_mutex_lock(&account->lock);
    int tmp = account->get();
    tmp += amount;
    account->put(tmp);
    pthread_mutex_unlock(&account->lock);
}

int withdraw(Acct *account, int amount){
    pthread_mutex_lock(&account->lock);
    int tmp = account->get();
    tmp -= amount;
    account->put(tmp);
    pthread_mutex_unlock(&account->lock);
}

void *person1_activity(void *param_bank){
    Bank *bank = (Bank *)param_bank;
    deposit(&(bank->array_acct[0]),5);
    withdraw(&(bank->array_acct[0]),1);
    deposit(&(bank->array_acct[1]), 10);
    withdraw(&(bank->array_acct[1]),3);
}
void *person2_activity(void *param_bank){
    Bank *bank = (Bank *)param_bank;
    withdraw(&(bank->array_acct[1]),8);
    deposit(&(bank->array_acct[0]),6);
    withdraw(&(bank->array_acct[0]),4);
    deposit(&(bank->array_acct[1]), 15);
}
void *person3_activity(void *param_bank){
    Bank *bank = (Bank *)param_bank;
    withdraw(&(bank->array_acct[0]),2);
    deposit(&(bank->array_acct[0]),12);
    deposit(&(bank->array_acct[1]), 13);
    withdraw(&(bank->array_acct[1]),7);
}

int main(){
    pthread_t person1;
    pthread_t person2;
    pthread_t person3;
    int num_accounts = 2;
    uint64_t addressofAcct[num_accounts];
    addressofAcct[0] = 2;
    addressofAcct[1]= 1000;
    Bank *bank = new Bank(num_accounts, addressofAcct);
    if(pthread_create(&person1,NULL, person1_activity, (void *)bank)){
        printf("Error creating thread\n");
        return 1;
    }
    if(pthread_create(&person2, NULL, person2_activity, (void *)bank)){
        printf("Error creating thread\n");
        return 1;
    }
    if(pthread_create(&person3, NULL, person3_activity, (void *)bank)){
        printf("Error creating thread\n");
        return 1;
    }
    if(pthread_join(person1, NULL)){
        printf("Error joining threads\n");
    }
    if(pthread_join(person2, NULL)){
        printf("Error joining threads\n");
    }
    if(pthread_join(person3, NULL)){
        printf("Error joining threads\n");
    }
    //printf("acct 1 = %d, acct2 = %d\n", bank->array_acct[0].get(), bank->array_acct[1].get());

    return 1;
}
