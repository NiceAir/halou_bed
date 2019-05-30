<?php

//登录成功返回200&cookie&username
//登录失败返回204&

function get_val($arg)
{
	$res = explode("=", $arg);
	return $res[1];
}

function deleteTag($str)
{
	$res = strip_tags($str);
	$res = htmlspecialchars($res);
	$res = urldecode($res);
	return $res;    
}


$res = explode("&", $argv[1]);
if(count($res) != 2) {
	echo "400&参数数量不对";
	exit();
}

$username = get_val($res[0]);
$passwd = get_val($res[1]);

$username = deleteTag($username);
$passwd = deleteTag($passwd);
$passwd = md5("baobao". $passwd . "xiangni");

$conn = new mysqli("127.0.0.1", "root", "123", "halou_bed");
if ($conn->connect_error) {
	echo "500&链接数据库失败";
	exit();
}
$sql = "select name from user where name='" . $username . "' and passwd='" . $passwd . "';"; 
$res = mysqli_query($conn, $sql);
if (mysqli_num_rows($res) <= 0)         //数据库里找不到数据（用户名或密码不对）
{
	echo "204&";
}
else          //找到了，登录成功
{
	
	$cooike = md5(time());
	echo "200&" . $cooike . "&" . $username;
}
$conn->close();
exit();

?>
