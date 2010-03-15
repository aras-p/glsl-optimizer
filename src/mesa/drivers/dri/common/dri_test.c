/* This is just supposed to make sure we get a reference to
   the driver entry symbol that the compiler doesn't optimize away */

extern char __driDriverExtensions[];

int main(int argc, char** argv)
{
   void* p = __driDriverExtensions;
   return (int)(unsigned long)p;
}
