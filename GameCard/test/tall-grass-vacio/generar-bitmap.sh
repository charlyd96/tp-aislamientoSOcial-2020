rm Bitmap.bin
touch Bitmap.bin
VAR1=""
for i in {1..4096}
do
	VAR1="${VAR1}0"
done
echo "$VAR1" > Bitmap.bin

