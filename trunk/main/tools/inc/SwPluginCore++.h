/***************************************************************************
    SwPluginCore.h++  -  C++ plugin declaration class for Piaf
                             -------------------
    begin                : 9 oct. 2008 13:24:50
    copyright            : (C) 2008 Pierre Lamot
    email                : pierre.lamot@openwide.fr
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SWPLUGINCORE_H_
#define SWPLUGINCORE_H_

#include <vector>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>

#include "SwPluginCore.h"

#include "TypeToPiaf.h"

struct PiafVar
{
	const char* name;
	int type;
	void* var;

	template<typename T>
	PiafVar(const char* varName, T& varRef):
			name(varName),
			type(TypeToPiafTraits<T>::value)
	{
		var = reinterpret_cast<void*>(&varRef);
	}
};

class IPiafPlugin
{
private:
	const char* _pluginName;

	std::vector<PiafVar> _pluginVars;

protected:
	swImageStruct* imIn;
	swImageStruct* imOut;

	IPiafPlugin(const char* name):
			_pluginName(name)
	{}

	template<typename T>
	void registervar(const char* name, T& var)
	{
		_pluginVars.push_back(PiafVar(name, var));
	}

	virtual void proc() = 0;

public:
	//called when a parametter changed
	virtual void onParameterChanged() {}
	friend class SwPluginCore2;
};

#define PIAF_PLUGIN(c, params)                          				\
  PIAF_PLUGIN_W_SEQ(c, BOOST_PP_CAT(PIAF_PARAM_TO_SEQ_X params, 0))


#define PIAF_PLUGIN_W_SEQ(c, params)									\
  struct c##_piaf_plug :												\
	public IPiafPlugin {												\
	  /*Ctor*/															\
	  c##_piaf_plug():													\
	  IPiafPlugin(#c),													\
      BOOST_PP_SEQ_FOR_EACH(PIAF_INIT_VAR, _, params)                   \
      _unused_param(false)												\
    {																	\
	 BOOST_PP_SEQ_FOR_EACH(PIAF_REGISTER_VAR, c, params)				\
	}                                                                  	\
	/*variable declaration*/ 											\
    BOOST_PP_SEQ_FOR_EACH(PIAF_DECL_VAR, _, params)                     \
    private:															\
    bool _unused_param;                                                 \
  };                                                                    \
  struct c : public c##_piaf_plug

#  define PIAF_PARAM_TO_SEQ_X(type, var, val) ((type, var, val)) PIAF_PARAM_TO_SEQ_Y
#  define PIAF_PARAM_TO_SEQ_Y(type, var, val) ((type, var, val)) PIAF_PARAM_TO_SEQ_X
#  define PIAF_PARAM_TO_SEQ_X0
#  define PIAF_PARAM_TO_SEQ_Y0

#  define PIAF_DECL_VAR(r, data, tuple) SUB_PIAF_DECL_VAR tuple
#    define SUB_PIAF_DECL_VAR(type, var, val) type var;

#  define PIAF_INIT_VAR(r, data, tuple) SUB_PIAF_INIT_VAR tuple
#    define SUB_PIAF_INIT_VAR(type, var, val) var(val) BOOST_PP_COMMA()

#  define PIAF_REGISTER_VAR(r, data, tuple) \
	PIAF_EVAL(SUB_PIAF_REGISTER_VAR tuple)
#    define PIAF_PARAM_ADD_PARAM(val, tuple) (val, PIAF_PARAM_ENUM tuple)
#      define PIAF_PARAM_ENUM(x,y,z) x,y,z
#    define PIAF_EVAL(value) value
#    define SUB_PIAF_REGISTER_VAR(type, var, val) registervar(#var, var);


#define PIAF_MAIN(category, subcategory, plugs) \
void signalhandler(int sig){ \
  fprintf(stderr, "================== RECEIVED SIGNAL %d = '%s' From process %d ==============\n", sig, sys_siglist[sig], getpid());\
  signal(sig, signalhandler);\
  if (sig != SIGUSR1)\
    exit(0);\
}\
int main(int, char **){\
  SwPluginCore2 plugin;\
  for (int i=0; i<NSIG; i++)\
    signal(i, signalhandler);\
  fprintf(stderr, "plugin registerCategory '%s'/'%s'...\n",category,subcategory);\
  plugin.registerCategory(category, subcategory);\
  fprintf(stderr, "registerFunctions...\n");\
  BOOST_PP_SEQ_FOR_EACH_I(PIAF_REGISTER_PLUGIN, _, plugs)\
  fprintf(stderr, "loop...\n");\
  plugin.loop();\
  fprintf(stderr, "exit(EXIT_SUCCESS). Bye.\n");\
  return EXIT_SUCCESS;\
}

#	define PIAF_REGISTER_PLUGIN(r, data, i, plugtype) \
		plugtype p##i;\
		p##i.onParameterChanged();\
		plugin.registerFunctions(&p##i);

class SwPluginCore2 : public SwPluginCore
{
	std::vector<IPiafPlugin*> plugs;

public:
	int registerFunctions(IPiafPlugin* plug)
	{
		plugs.push_back(plug);
		NbFunctions = plugs.size();
	}

protected:
	int setFunctionParameters(char *frame, int len);
	int processFunction(char *frame, int len);
	char * sendFunctions();
	int sendFunctionDescriptor(int funcnum);

	int treatFrame(char * framebuffer, int framesize);
};

#endif /* SWPLUGINCORE_H_ */

