ip='192.168.1.107'
registerFile="register.txt"
loginFile="login.txt"
echo **************************************进入主页开始*******************************
#ab -c 1000 -n 1000 -r -k http://$ip:9999/
echo **************************************进入主页结束*******************************
echo 

echo **************************************进入登录页开始*******************************
#ab -c 1000 -n 1000 -r -k http://$ip:9999/log_about/login.html
echo ************************************* 进入登录页结束*******************************
echo 

echo **************************************进入注册页开始*******************************
#ab -c 1000 -n 1000 -r -k http://$ip:9999/log_about/register.html
echo **************************************进入注册页结束*******************************
echo 


echo **************************************请求图片开始*****************************
#ab -c 1000 -n 1000 -r -k http://$ip:9999/log_about/images/wqgwqgwwwboqfufo.jpg
echo **************************************请求图片结束*******************************
echo 

echo **************************************用户登录开始*******************************
ab -c 100 -n 100 -r -k -p $loginFile   -T 'application/x-www-form-urlencoded'    http://$ip:9999/log_about/login
echo **************************************用户登录结束*******************************
echo 

echo **************************************用户注册开始*******************************
#ab -c 50 -n 100 -r -k -p $registerFile   -T 'application/x-www-form-urlencoded'    http://$ip:9999/log_about/register
echo **************************************用户注册结束*******************************
echo 
