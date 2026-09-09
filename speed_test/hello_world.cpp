
#include <iostream>

class test_class
{
  protected:
  
  std::string test_str = "VOID";
  
  void set_test_str(std::string test_str_input)
  {
    test_str = test_str_input;
  }
  
  public:
  
  void call_set_test_str(std::string call_test_str_input)
  {
    set_test_str(call_test_str_input);
  }
};

int main()
{
    signed long long int current_number = 0;
    signed long long int target_number = 100000;

    while (current_number < target_number)
    {
      test_class local_test_object;
        
      local_test_object.call_set_test_str("hello");
        
      current_number = current_number + 1;
    }

    return(0);
}
