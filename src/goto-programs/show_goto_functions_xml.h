/*******************************************************************\

Module: Goto Program

Author: Thomas Kiley

\*******************************************************************/

#ifndef CPROVER_GOTO_PROGRAMS_SHOW_GOTO_FUNCTIONS_XML_H
#define CPROVER_GOTO_PROGRAMS_SHOW_GOTO_FUNCTIONS_XML_H

#include <util/xml.h>

class goto_functionst;
class namespacet;

class show_goto_functions_xmlt
{
public:
  explicit show_goto_functions_xmlt(const namespacet &ns);

  xmlt convert(const goto_functionst &goto_functions);
  void operator()(
    const goto_functionst &goto_functions, std::ostream &out, bool append=true);

private:
  const namespacet &ns;
};

#endif // CPROVER_GOTO_PROGRAMS_SHOW_GOTO_FUNCTIONS_XML_H
