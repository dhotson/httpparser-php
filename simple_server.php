<?php

/*
 * A simple pre-forking web server
 */

if (!($sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)))
{
	echo socket_strerror(socket_last_error())."\n";
	exit(1);
}

if (!socket_set_option($sock, SOL_SOCKET, SO_REUSEADDR, 1))
{
	echo socket_strerror(socket_last_error())."\n";
	exit(1);
}

if (!socket_bind($sock, '0.0.0.0', 9000))
{
	echo socket_strerror(socket_last_error())."\n";
	exit(1);
}

if (!socket_listen($sock))
{
	echo socket_strerror(socket_last_error())."\n";
	exit(1);
}

$maxConcurrency = 5;
$procs = array();

while(true)
{
	if (-1 == ($pid = pcntl_fork()))
	{
		throw new Exception('Could not fork');
	}
	else if ($pid == 0)
	{
		server($sock);
		exit(0);
	}
	else // parent process
	{
		$procs[$pid] = $pid;
	}

	if (count($procs) >= $maxConcurrency)
	{
		if (-1 == ($pid = pcntl_wait($status)))
		{
			throw new Exception('Something went wrong in pcntl_wait');
		}

		$exitStatus = pcntl_wexitstatus($status);
		unset($procs[$pid]);
	}
}

function server($sock)
{
	while ($client = socket_accept($sock))
	{
		$buf = '';
		$nparsed = 0;

		$h = new HttpParser();

		while ($data = socket_read($client, 10, PHP_BINARY_READ))
		{
			$buf .= $data;
			$nparsed = $h->execute($buf, $nparsed);

			if ($h->hasError())
			{
				socket_close($client);
				continue;
			}

			if ($h->isFinished())
			{
				break;
			}
		}

		$env = $h->getEnvironment();

		$result = '<form action="" method="post"><input type="submit" name="testvar" value="Testing!" /></form><pre>';
		foreach ($env as $k => $v)
			$result .= sprintf("%s -> '%s'\n", $k, $v);

		$response = join(
			"\r\n",
			array(
				'HTTP/1.1 200 OK',
				'Content-Type: text/html',
				'Content-Length: '.strlen($result),
				'',
				$result));
		$n = socket_write($client, $response);

		socket_close($client);
	}
}
