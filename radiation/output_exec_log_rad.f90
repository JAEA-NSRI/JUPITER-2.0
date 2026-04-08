!*******************************************************************************
!>    Logファイルの出力を行う(KKE2013.03)
!*******************************************************************************
subroutine output_exec_log_rad(isub)

use MODULE_LOG

implicit none

integer(4) :: isub
integer(4) :: i
logical,save :: first=.true.

if(first) then
	open(110,file='radiation_exec.log',action='write',status='replace') !exe_logのリプレース
	close(110)
	first=.false.
	LogStr(:)='\\\\'
endif

do i = 1, NumLogLine
	if(LogStr(i)(1:4).eq.'\\\\') exit
enddo
open(110,file='radiation_exec.log',position='append')
do i = 1, NumLogLine
	if(LogStr(i)(1:4).eq.'\\\\') exit
	write(110,*) trim(LogStr(i))
enddo
close(110)

! 独立動作時には, 進捗を画面にも出力する
if(isub .eq. 0) then
	do i = 1, NumLogLine
		if(LogStr(i)(1:4).eq.'\\\\') exit
		write(6,*) trim(LogStr(i))
	enddo
endif

LogStr(:)='\\\\'

return
end