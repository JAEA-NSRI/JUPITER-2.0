! # をコメントとするテキストデータから, 一行読み込む関数 (引数 = ファイルID, 戻り値 = 一行の内容)
character(255) function readline(input_id)

	implicit none
	
	integer:: input_id				! 引数リスト
!	integer:: line= 0
	integer, save :: line
!	character(255):: readline     ! 関数戻り値


	readline(:) = ''
	do
		line= line+ 1
		read(input_id, '(a)', end= 100, err= 200) readline
		if(readline(1:1) .ne. '#') return
	enddo

100	continue
    write(6, *) "File does not contain effective data"
	pause
	stop
	
200	continue
    write(6, *) "File error at line: ", line
	pause
	stop
	
end