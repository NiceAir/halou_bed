<?php
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
if(count($res) != 4) {
	echo "参数数量错误&400";
	exit();
}
$username = get_val($res[0]);
$passwd1 = get_val($res[1]);
$passwd2 = get_val($res[2]);
$email = get_val($res[3]);

$username = deleteTag($username);
$passwd1 = deleteTag($passwd1);
$passwd2 = deleteTag($passwd2);
$email = deleteTag($email);


if($passwd1 == $passwd2)
	$passwd1 = md5("baobao". $passwd1 . "xiangni");
else {
	echo "两次密码不一致&400";
	exit();
}
	
$conn = new mysqli("127.0.0.1", "root", "123", "halou_bed");
if ($conn->connect_error) {
	echo "链接数据库失败&500";
	exit();	
} 
$sql = "select name from user where name='" . $username . "';";
$res = mysqli_query($conn, $sql);
if (mysqli_num_rows($res) > 0)
{
	echo "该用户已存在".$username. "&200";
	$conn->close();
	exit();
}
$sql = sprintf("insert into user(name, passwd, email) values('%s', '%s', '%s');", $username, $passwd1, $email);
$res = mysqli_query($conn, $sql);
if($res)
	echo "注册成功" . $username . "&200";
else
	echo "注册失败&500";
$conn->close();

echo $username . "  " . $passwd1 . "  " . strlen($passwd1) . "  " . $email;
?>
