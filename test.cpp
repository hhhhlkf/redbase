#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cstdlib>
#include <cmath>
using namespace std;
int main(int argc, char const *argv[])
{
    ofstream ofs;
    ofs.open("test.txt", ios::out);
    srand(time(0));
    int min = 0;
    int max = 100000;
    for (int i = 0; i < 1000; i++)
    {
        ofs << i + 1 << ",";
        string a;
        for (int i = 0; i < 9; i++)
        {
            char c = rand() % (122 - 97) + 97;
            cout << c << endl;
            a += (c);
        }
        // cout << rand() % 256 << endl;
        int r = (rand() % (max - min + 1)) + min;
        ofs << a << "," << r << endl;
    }
    ofs.close();
    // cout << ceil(1.0 / 9) << endl;
    return 0;
}
