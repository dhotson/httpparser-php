#include "httpparser.h"
#include "http11_parser.h"

zend_object_handlers http_parser_object_handlers;

typedef struct http_parser_object
{
	zend_object std;
	http_parser *hp;
	zval *env;
} http_parser_object;

void http_parser_free_storage(void *object TSRMLS_DC)
{
	http_parser_object *obj = (http_parser_object *)object;
	efree(obj->hp);

	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);

	efree(obj);
}

zend_object_value http_parser_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	http_parser_object *obj = (http_parser_object *)emalloc(sizeof(http_parser_object));
	memset(obj, 0, sizeof(http_parser_object));
	obj->std.ce = type;

	ALLOC_HASHTABLE(obj->std.properties);
	zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(obj->std.properties, &type->default_properties,
		(copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(obj, NULL,
		http_parser_free_storage, NULL TSRMLS_CC);
	retval.handlers = &http_parser_object_handlers;

	return retval;
}

void http_field_callback(void *data, const char *field_name, size_t field_name_len, const char *field_value, size_t field_value_size)
{
	char key[256] = "HTTP_";
	int max = 256 - strlen(key) - 1;
	int n = field_name_len < max
		? field_name_len
		: max;

	strncat(key, field_name, n);

	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)field_value, field_value_size, 1);
}

void request_uri_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "REQUEST_URI";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void fragment_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "FRAGMENT";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void request_path_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "PATH_INFO";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void query_string_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "QUERY_STRING";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void http_version_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "HTTP_VERSION";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void request_method_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "REQUEST_METHOD";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

void header_done_callback(void *data, const char * buffer, size_t buffer_len)
{
	char *key = "REQUEST_BODY";
	add_assoc_stringl_ex(data, key, strlen(key)+1, (char*)buffer, buffer_len, 1);
}

zend_class_entry *http_parser_ce;

PHP_METHOD(HttpParser, __construct)
{
	zval *object = getThis();
	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(object TSRMLS_CC);

	zval *env;
	ALLOC_INIT_ZVAL(env);
	array_init(env);

	obj->env = env;

	http_parser *hp = emalloc(sizeof(http_parser));
	hp->http_field = http_field_callback;
	hp->request_uri = request_uri_callback;
	hp->fragment = fragment_callback;
	hp->request_path = request_path_callback;
	hp->query_string = query_string_callback;
	hp->http_version = http_version_callback;
	hp->header_done = header_done_callback;
	hp->request_method = request_method_callback;
	http_parser_init(hp);
	obj->hp = hp;
	obj->hp->data = obj->env;
}

PHP_METHOD(HttpParser, __destruct)
{
	zval *object = getThis();
	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(object TSRMLS_CC);

	zval_ptr_dtor(&obj->env);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_httpparser_execute, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, http_string)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

PHP_METHOD(HttpParser, execute)
{
	char *data;
	int data_len;
	long offset = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &data, &data_len, &offset) == FAILURE) {
		RETURN_NULL();
	}

	if (offset < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "negative offsets are not allowed");
		RETURN_NULL();
	}

	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	http_parser_execute(obj->hp, data, data_len, offset);

	RETURN_LONG(http_parser_nread(obj->hp));
}

PHP_METHOD(HttpParser, hasError)
{
	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (http_parser_has_error(obj->hp)) {
		RETURN_TRUE;
	}

	RETURN_FALSE;
}

PHP_METHOD(HttpParser, isFinished)
{
	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (http_parser_is_finished(obj->hp)) {
		RETURN_TRUE;
	}

	RETURN_FALSE;
}

PHP_METHOD(HttpParser, getEnvironment)
{
	http_parser_object *obj = (http_parser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_ZVAL(obj->env, 1, 0);
}


function_entry http_parser_methods[] = {
	PHP_ME(HttpParser, __construct,    NULL,                       ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(HttpParser, __destruct,     NULL,                       ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
	PHP_ME(HttpParser, execute,        arginfo_httpparser_execute, ZEND_ACC_PUBLIC)
	PHP_ME(HttpParser, hasError,       NULL,                       ZEND_ACC_PUBLIC)
	PHP_ME(HttpParser, isFinished,     NULL,                       ZEND_ACC_PUBLIC)
	PHP_ME(HttpParser, getEnvironment, NULL,                       ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(httpparser)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "HttpParser", http_parser_methods);
	http_parser_ce = zend_register_internal_class(&ce TSRMLS_CC);
	http_parser_ce->create_object = http_parser_create_handler;
	memcpy(&http_parser_object_handlers,
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	http_parser_object_handlers.clone_obj = NULL;
	return SUCCESS;
}


zend_module_entry http_parser_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_HTTP_PARSER_EXTNAME,
    NULL,        /* Functions */
    PHP_MINIT(httpparser),        /* MINIT */
    NULL,        /* MSHUTDOWN */
    NULL,        /* RINIT */
    NULL,        /* RSHUTDOWN */
    NULL,        /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_HTTP_PARSER_EXTVER,
#endif
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(http_parser)
