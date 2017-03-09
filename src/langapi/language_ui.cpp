/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <fstream>
#include <memory>
#include <iostream>

#include <util/namespace.h>
#include <util/language.h>
#include <util/cmdline.h>
#include <util/unicode.h>
#include <util/mp_arith.h>
#include <util/arith_tools.h>
#include <util/std_types.h>

#include "language_ui.h"
#include "mode.h"

/*******************************************************************\

Function: language_uit::language_uit

  Inputs:

 Outputs:

 Purpose: Constructor

\*******************************************************************/

language_uit::language_uit(
  const cmdlinet &__cmdline,
  ui_message_handlert &_ui_message_handler):
  ui_message_handler(_ui_message_handler),
  _cmdline(__cmdline)
{
  set_message_handler(ui_message_handler);
}

/*******************************************************************\

Function: language_uit::~language_uit

  Inputs:

 Outputs:

 Purpose: Destructor

\*******************************************************************/

language_uit::~language_uit()
{
}

/*******************************************************************\

Function: language_uit::parse()

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool language_uit::parse()
{
  for(unsigned i=0; i<_cmdline.args.size(); i++)
  {
    if(parse(_cmdline.args[i]))
      return true;
  }

  return false;
}

/*******************************************************************\

Function: language_uit::parse()

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool language_uit::parse(const std::string &filename)
{
  #ifdef _MSC_VER
  std::ifstream infile(widen(filename));
  #else
  std::ifstream infile(filename);
  #endif

  if(!infile)
  {
    error() << "failed to open input file `" << filename << "'" << eom;
    return true;
  }

  std::pair<language_filest::file_mapt::iterator, bool>
    result=language_files.file_map.insert(
      std::pair<std::string, language_filet>(filename, language_filet()));

  language_filet &lf=result.first->second;

  lf.filename=filename;
  lf.language=get_language_from_filename(filename);

  if(lf.language==NULL)
  {
    error("failed to figure out type of file", filename);
    return true;
  }

  languaget &language=*lf.language;
  language.set_message_handler(get_message_handler());
  language.get_language_options(_cmdline);

  status() << "Parsing " << filename << eom;

  if(language.parse(infile, filename))
  {
    if(get_ui()==ui_message_handlert::PLAIN)
      std::cerr << "PARSING ERROR" << std::endl;

    return true;
  }

  lf.get_modules();

  return false;
}

/*******************************************************************\

Function: language_uit::typecheck

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool language_uit::typecheck()
{
  status() << "Converting" << eom;

  language_files.set_message_handler(*message_handler);

  if(language_files.typecheck(symbol_table))
  {
    if(get_ui()==ui_message_handlert::PLAIN)
      std::cerr << "CONVERSION ERROR" << std::endl;

    return true;
  }

  return false;
}

/*******************************************************************\

Function: language_uit::final

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool language_uit::final()
{
  language_files.set_message_handler(*message_handler);

  if(language_files.final(symbol_table))
  {
    if(get_ui()==ui_message_handlert::PLAIN)
      std::cerr << "CONVERSION ERROR" << std::endl;

    return true;
  }

  return false;
}

/*******************************************************************\

Function: language_uit::show_symbol_table

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void language_uit::show_symbol_table(bool brief)
{
  switch(get_ui())
  {
  case ui_message_handlert::PLAIN:
    show_symbol_table_plain(std::cout, brief);
    break;

  case ui_message_handlert::XML_UI:
    show_symbol_table_xml_ui(brief);
    break;

  default:
    error() << "cannot show symbol table in this format" << eom;
  }
}

/*******************************************************************\

Function: language_uit::show_symbol_table_xml_ui

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void language_uit::show_symbol_table_xml_ui(bool brief)
{
  error() << "cannot show symbol table in this format" << eom;
}

/*******************************************************************\

Function: language_uit::show_symbol_table_plain

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void language_uit::show_symbol_table_plain(
  std::ostream &out,
  bool brief)
{
  if(!brief)
    out << '\n' << "Symbols:" << '\n' << std::endl;

  // we want to sort alphabetically
  std::set<std::string> symbols;

  forall_symbols(it, symbol_table.symbols)
    symbols.insert(id2string(it->first));

  const namespacet ns(symbol_table);

  for(const std::string &id : symbols)
  {
    const symbolt &symbol=ns.lookup(id);

    languaget *ptr;

    if(symbol.mode=="")
      ptr=get_default_language();
    else
    {
      ptr=get_language_from_mode(symbol.mode);
      if(ptr==NULL)
        throw "symbol "+id2string(symbol.name)+" has unknown mode";
    }

    std::unique_ptr<languaget> p(ptr);
    std::string type_str, value_str;

    if(symbol.type.is_not_nil())
      p->from_type(symbol.type, type_str, ns);

    if(symbol.value.is_not_nil())
      p->from_expr(symbol.value, value_str, ns);

    if(brief)
    {
      out << symbol.name << " " << type_str << std::endl;
      continue;
    }

    out << "Symbol......: " << symbol.name << '\n' << std::flush;
    out << "Pretty name.: " << symbol.pretty_name << '\n';
    out << "Module......: " << symbol.module << '\n';
    out << "Base name...: " << symbol.base_name << '\n';
    out << "Mode........: " << symbol.mode << '\n';
    out << "Type........: " << type_str << '\n';
    out << "Value.......: " << value_str << '\n';
    out << "Flags.......:";

    if(symbol.is_lvalue)
      out << " lvalue";
    if(symbol.is_static_lifetime)
      out << " static_lifetime";
    if(symbol.is_thread_local)
      out << " thread_local";
    if(symbol.is_file_local)
      out << " file_local";
    if(symbol.is_type)
      out << " type";
    if(symbol.is_extern)
      out << " extern";
    if(symbol.is_input)
      out << " input";
    if(symbol.is_output)
      out << " output";
    if(symbol.is_macro)
      out << " macro";
    if(symbol.is_parameter)
      out << " parameter";
    if(symbol.is_auxiliary)
      out << " auxiliary";
    if(symbol.is_weak)
      out << " weak";
    if(symbol.is_property)
      out << " property";
    if(symbol.is_state_var)
      out << " state_var";
    if(symbol.is_exported)
      out << " exported";
    if(symbol.is_volatile)
      out << " volatile";

    out << '\n';
    out << "Location....: " << symbol.location << '\n';

    out << '\n' << std::flush;
  }
}

/*******************************************************************\

Function: language_uit::list_extended_symbols

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void language_uit::list_extended_symbols(
  const namespacet ns,
  const std::string symbol,
  const typet type,
  std::unique_ptr<languaget> &p,
  std::ostream &out)
{
  if (type.id()==ID_array)
  {
    const exprt &size_expr=static_cast<const exprt &>(type.find(ID_size));
    mp_integer mp_count;
    to_integer(size_expr, mp_count);
    unsigned count=integer2unsigned(mp_count);
    for(unsigned int i=0;i<count;i++)
    {
      std::stringstream buffer;
      buffer << symbol << "[" << i << "]";
      list_extended_symbols(ns,buffer.str(),ns.follow(type.subtype()),p,out);
    }
  }
  else if (type.id()==ID_struct)
  {
    const struct_typet &struct_type=to_struct_type(type);
    const struct_typet::componentst &components=struct_type.components();
    for(struct_typet::componentst::const_iterator it=components.begin();
	          it!=components.end();++it)
	  {
      std::stringstream buffer;
      buffer << symbol << "." << it->get_name();
      list_extended_symbols(ns,buffer.str(),ns.follow(it->type()),p,out);
	  }
  }
  else 
  {
    std::string type_str;
    p->from_type(type, type_str, ns);
    out << "{&" << symbol << "," << type_str << "};" << std::endl;
  }
}



/*******************************************************************\

Function: language_uit::build_array_from_static_symbol_table

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/
unsigned language_uit::build_array_from_static_symbol_table(
    std::ostream &address_out, 
    std::ostream &type_out)
{
  unsigned count = 0;
  std::stringstream types;
  std::stringstream addresses;
  const namespacet ns(symbol_table);

  std::set<std::string> symbols;
  forall_symbols(it, symbol_table.symbols)
    symbols.insert(id2string(it->first));
 
  for(const std::string &id : symbols)
  {
    const symbolt &symbol=ns.lookup(id);
    languaget *ptr;
    if(symbol.mode=="")
      ptr=get_default_language();
    else
    {
      ptr=get_language_from_mode(symbol.mode);
      if(ptr == nullptr) 
        throw "symbol "+id2string(symbol.name)+" has unknown mode";
    }
  
    std::unique_ptr<languaget> p(ptr);
    std::string type_str;
    std::string value_str;

    if(symbol.type.is_not_nil())
      p->from_type(symbol.type, type_str, ns);

    if(symbol.value.is_not_nil())
      p->from_expr(symbol.value, value_str, ns);
  
    if((symbol.is_static_lifetime) && (!symbol.location.is_built_in()))
    {
      const typet type = ns.follow(symbol.type);
    	std::stringstream buffer;
    	buffer << symbol.base_name;
      build_entry(ns,type,p,buffer.str(),addresses,types,count);
    }
    continue;
  }
  if(count > 0)
  {
    address_out << "void* __input_addresses[] = {" 
                  << addresses.str()
                  << "};";
    type_out << "int __input_type[] = {"
                 << types.str()
                 << "};";
  }
  else
  {
    address_out << "static void* __input_addresses = 0;";
    type_out  << "static int* __input_type = 0;";
  }
  return count;
}

/*******************************************************************\

Function: language_uit::build_entry

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/
void language_uit::build_entry(const namespacet ns,
    const typet type, 
    std::unique_ptr<languaget> &p,
    const std::string name, 
    std::ostream &addresses,
    std::ostream &types,
    unsigned &count)
{
  if (type.id() == ID_array)
  {
    const exprt &size_expr=static_cast<const exprt &>(type.find(ID_size));
    mp_integer mp_count;
    to_integer(size_expr, mp_count);
    unsigned count=integer2unsigned(mp_count);
    for(unsigned int i=0;i<count;i++)
    {
      std::stringstream buffer;
      buffer << name  << "[" << i << "]";
      build_entry(ns,ns.follow(type.subtype()),p,
          buffer.str(),addresses,types,count);
    }
  }
  else if (type.id() == ID_struct)
  {
    const struct_typet &struct_type=to_struct_type(type);
    const struct_typet::componentst &components=struct_type.components();
    for(struct_typet::componentst::const_iterator it=components.begin();
        it!=components.end();++it)
    {
      std::stringstream buffer;
      buffer << name << "." << it->get_name();
      build_entry(ns,ns.follow(type.subtype()),p,
          buffer.str(),addresses,types,count);
    }
  }
  else
  {
    if(name == "argc")
      return;
    std::string type_str;
    p->from_type(type, type_str, ns);
    if(count != 0)
    {
      types << ",";
      addresses << ",";
    }
    addresses << "&" << name;
    static const std::unordered_map<std::string,unsigned> type_map 
      = {{"unsigned int",0},{"signed int",1},
         {"char",2},{"unsigned long int", 3},{"signed long long int",4},
         {"unsigned long long int",5},{"float",6},{"double",7},
         {"signed char",2},{"unsigned char",8},{"signed short int",9},
         {"unsigned short int",10},{"signed long int",11}};
    auto it = type_map.find(type_str);
    if(it == type_map.end())
      //used for debugging.
      types << type_str;
    else
      types << it->second;
    count += 1;
  }
}
