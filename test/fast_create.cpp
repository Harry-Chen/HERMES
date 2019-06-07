#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

const string base = "batch_file.";

int main(int argc, char **argv) {
    if(argc != 2) {
        cout<<"Usage: fast_create [COUNT]"<<endl;
        return 1;
    }

    stringstream ss(argv[1]);
    uint64_t count;
    ss>>count;

    cout<<"Creating "<<count<<" files..."<<endl;

    for(size_t i = 0; i<count; ++i) {
        ofstream(base + to_string(i)); // Immediately drops
        if((i + 1) % 10000 == 0) cout<<i + 1<<" / "<<count<<endl;
    }
}
