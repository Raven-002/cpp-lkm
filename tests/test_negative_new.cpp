// Used for negative link test. Should fail to link due to undefined reference to operator new
int main()
{
    int* p = new int{42};
    return *p;
}
