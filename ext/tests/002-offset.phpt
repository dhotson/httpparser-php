--TEST--
Simple HTTP-request with Offset
--SKIPIF--
<?php if (!extension_loaded("httpparser")) print "skip"; ?>
--FILE--
<?php
// empty body
$parser = new HttpParser();
$nparsed = $parser->execute("testGET / HTTP/1.1\r\nHost: example.com\r\n\r\n", 4);
var_dump($parser->getEnvironment());

$parser = new HttpParser();
$nparsed = $parser->execute("testGET / HTTP/1.1\r\nHost: example.com\r\n\r\n", -4);
var_dump($parser->getEnvironment());
?>
==DONE==
--EXPECTF--
array(6) {
  ["REQUEST_METHOD"]=>
  string(3) "GET"
  ["PATH_INFO"]=>
  string(1) "/"
  ["REQUEST_URI"]=>
  string(1) "/"
  ["HTTP_VERSION"]=>
  string(8) "HTTP/1.1"
  ["HTTP_HOST"]=>
  string(11) "example.com"
  ["REQUEST_BODY"]=>
  string(0) ""
}

Warning: HttpParser::execute(): negative offsets are not allowed in %s on line %d
array(0) {
}
==DONE==
