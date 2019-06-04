i=0
while [ $i -lt 100 ]
do
	username="呵呵"$i
	passwd1="123"
	passwd2="123"
	email="1362605516@qq.com"
	echo username=$username\&passwd1=$passwd1\&passwd2=$passwd2\&email=$email
	let i=i+1
done

