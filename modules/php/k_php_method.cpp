/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2009 Appcelerator, Inc. All Rights Reserved.
 */
#include "php_module.h"

namespace kroll {

	KPHPMethod::KPHPMethod(zval* object, const char* methodName) :
		object(object),
		functionTable(0),
		methodName(strdup(methodName))
	{
		zval_addref_p(object);
	}
	
	KPHPMethod::KPHPMethod(HashTable* functionTable, const char *functionName) :
		object(0),
		functionTable(functionTable),
		methodName(strdup(functionName))
	{
	}
	
	KPHPMethod::KPHPMethod(const char *globalFunctionName TSRMLS_DC) :
		object(0),
		functionTable(EG(function_table)),
		methodName(strdup(globalFunctionName))
	{
	}

	KPHPMethod::~KPHPMethod()
	{
		if (object)
		{
			zval_delref_p(object);
		}
		
		free(methodName);
	}

	SharedValue KPHPMethod::Call(const ValueList& args)
	{
		TSRMLS_FETCH();

		zval* zReturnValue = NULL;
		zend_class_entry* classEntry = NULL;
		
		if (object)
		{
			classEntry = Z_OBJCE_P(object);
		}

		// Zendify method name.
		zval zMethodName;
		ZVAL_STRINGL(&zMethodName, methodName, strlen(methodName), 0);

		// Convert the arguments to PHP zvals.
		zval** zargs = new zval*[args.size()];
		for (int i = 0; i < args.size(); i++)
		{
			zargs[i] = new zval;
			PHPUtils::ToPHPValue(args[i], zargs + i);
		}

		// Construct a zend_fcall_info structure which describes
		// the method call we are about to invoke.
		zend_fcall_info callInfo;
		callInfo.size = sizeof(zend_fcall_info);
		if (object)
		{
			callInfo.object_ptr = object;
		}
		callInfo.function_name = &zMethodName;
		callInfo.retval_ptr_ptr = &zReturnValue;
		callInfo.param_count = args.size();;
		callInfo.params = &zargs;
		callInfo.no_separation = 1;
		callInfo.symbol_table = NULL;
		if (object)
		{
			callInfo.function_table = &classEntry->function_table;
		}
		else
		{
			callInfo.function_table = functionTable;
		}

		int result = zend_call_function(&callInfo, NULL TSRMLS_CC);

		for (int i = 0; i < args.size(); i++)
		{
			delete zargs[i];
		}
		delete [] zargs;

		if (result == FAILURE)
		{
			throw ValueException::FromFormat("Couldn't execute method %s::%s", 
				classEntry->name, methodName);
			return Value::Undefined;
		}
		else if (zReturnValue)
		{
			SharedValue returnValue(PHPUtils::ToKrollValue(zReturnValue TSRMLS_CC));
			zval_ptr_dtor(&zReturnValue);
			return returnValue;
		}
		else
		{
			return Value::Undefined;
		}
	}

	void KPHPMethod::Set(const char *name, SharedValue value)
	{
		// TODO: PHP methods do not have properties. Should we should set
		// them on a StaticBoundObject here?
	}

	SharedValue KPHPMethod::Get(const char *name)
	{
		// TODO: PHP methods do not have properties. Should we should get
		// them from a StaticBoundObject here?
		return Value::Undefined;
	}

	bool KPHPMethod::Equals(SharedKObject other)
	{
		AutoPtr<KPHPMethod> phpOther = other.cast<KPHPMethod>();
		if (phpOther.isNull())
			return false;
		
		TSRMLS_FETCH();
		return PHPUtils::PHPObjectsEqual(this->ToPHP(), phpOther->ToPHP() TSRMLS_CC);
	}

	SharedString KPHPMethod::DisplayString(int levels)
	{
		std::string* displayString = new std::string("KPHPMethod: ");
		displayString->append(methodName);
		return displayString;
	}

	SharedStringList KPHPMethod::GetPropertyNames()
	{
		return new StringList();
	}

	zval* KPHPMethod::ToPHP()
	{
		return this->object;
	}
}