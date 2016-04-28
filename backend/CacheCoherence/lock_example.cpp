class Acct {
    int balance;
    uint64_t addr;

    public:
        Lock lock;
        int get();
        void put(int);
}

int Acct::get(){
    return balance;
}

void Acct::put(int amount){
    balance = amount;
}

int deposit(Acct account, int amount){
    lock(account.lock);
    int tmp = account.get();
    tmp += amount;
    account.put(tmp);
    unlock(account.lock);
}

int withdraw(Acct account, int amount){
    lock(account.lock);
    int tmp = account.get();
    tmp -= amount;
    account.put(tmp);
    unlock(account.lock);
}
