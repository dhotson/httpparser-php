PHP_ARG_ENABLE(httpparser,
    [Whether to enable the "httpparser" extension],
    [  --enable-httpparser      Enable "httpparser" extension support])

if test $PHP_HTTPPARSER != "no"; then
    PHP_NEW_EXTENSION(httpparser, httpparser.c http11_parser.c, $ext_shared)
fi

