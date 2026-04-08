!*******************************************************************************
!>    module定義集(KKE2013.01)
!*******************************************************************************

!>構造体定義
module MODULE_STRUCT
	implicit none !$$$ 2014 2/12 弘中追加
	!>開始終了番号
	type begin_end
		integer(4) bg !<開始番号
		integer(4) ed !<終了番号
	end type
	!>最大最小値(実数)
	type max_min
		real(8) max !<最大値
		real(8) min !<最小値
	end type
	!>最大最小値(整数)
	type imax_imin
		integer(4) imax !<最大値
		integer(4) imin !<最小値
	end type
	!>3次元ベクトル
	type vector3d
		real(8) x
		real(8) y
		real(8) z
	end type
	!>出力バッファ要素(4Byte)
	type bindata4
		union
			map
				integer(4) int !<整数
			endmap
			map
				real(4) flt !<端精度実数
			endmap
			map
				character(4) cha !<文字列
			endmap
		end union
	end type
	!>出力バッファ要素(1Byte)
	type bindata1
		union
			map
				integer(1) int !<整数
			endmap
			map
				character(1) cha !<文字
			endmap
		end union
	end type
	!>時刻歴波形データ
	type wavedata
		integer(4) :: ityp !<データ種別
		integer(4) :: nsmp !<時刻歴波形の標本数
		integer(4) :: icur !<現在参照標本番号
		real(8) :: curr    !<現在時刻値
		real(8), pointer :: time(:) !<時刻歴波形の時間データ
		real(8), pointer :: wave(:) !<時刻歴波形の値データ
	end type
	!>物性データ
	type material
		real(8) :: phase_change_temp !<相変化温度
		real(8) :: heat_latent !<潜熱(温度回復法により相変化計算するので温度依存性は考慮しない)
		real(8) :: heat_oxidation, A, Ea, mass_ratio !<$$$ 酸化反応熱, Arrhenius 式のパラメータ, 反応原料物, 生成物の質量換算因子
		type(max_min) :: temp !<温度テーブルの定義範囲(各パラメータ共通)//(温度分割幅は固定値であり(max-min)/dble(num)から計算される)	
		!テーブルプロット数(一定値の場合は1)
		integer(4) :: num_rho
		integer(4) :: num_vis
		integer(4) :: num_ald
		integer(4) :: num_cv
		integer(4) :: num_alp
		integer(4) :: num_sgm
		integer(4) :: num_ang
		integer(4) :: num_emi
		!以下は物性テーブル
		real(8),pointer :: rho_table(:) !<密度
		real(8),pointer :: vis_table(:) !<粘度
		real(8),pointer :: ald_table(:) !<熱伝導率
		real(8),pointer :: cv_table(:)  !<比熱
		real(8),pointer :: alp_table(:) !<圧縮率
		real(8),pointer :: sgm_table(:) !<表面張力係数(ポテンシャル係数)
		real(8),pointer :: ang_table(:) !<壁面接触角(degree)
		real(8),pointer :: emi_table(:) !$$$< 射出率: 弘中追加
		!以下は物性固定値
		real(8) :: rho !<密度
		real(8) :: vis !<粘度(動粘度から粘度へ変更する)
		real(8) :: ald !<熱伝導率
		real(8) :: cv  !<比熱
		real(8) :: alp !<圧縮率
		real(8) :: sgm !<表面張力係数(ポテンシャル係数)
		real(8) :: ang !<壁面接触角(degree)
		real(8) :: emi !$$$< 射出率: 弘中追加
	end type
end module

!>ログ出力の配列
module MODULE_LOG
	implicit none !$$$ 2014 2/12 弘中追加
	integer(4) :: NumLogLine                 !<logのバッファリング可能な最大行数
	parameter(NumLogLine=15)
	character(len=255) :: LogStr(NumLogLine) !<log出力用文字バッファ
end module MODULE_LOG

!>粒子構造体
module MODULE_PARTICLE
	use MODULE_STRUCT

	implicit none !$$$ 2014 2/12 弘中追加
	!粒子変数
	integer(4), save :: number_particle
	!プロパティ
!	integer(4), pointer :: id(:)
	integer(4), pointer :: ityp(:)
	integer(4), pointer :: imat(:)
	integer(4), pointer :: ibc(:)
	integer(4), pointer :: ibct(:)
	integer(4), pointer :: isld(:)
	!一時物性
	real(8), pointer :: rho(:)
	real(8), pointer :: rho_ini(:) !<$$$ 粒子の初期密度 (物性としての)
	real(8), pointer :: vis(:)
	real(8), pointer :: ald(:)
	real(8), pointer :: sgm(:)
	real(8), pointer :: ang(:)
	real(8), pointer :: cv(:)
	real(8), pointer :: alp(:)
	!各種状態量
	real(8), pointer :: prs(:)
	real(8), pointer :: tmp(:)
	real(8), pointer :: oxidation_mass(:) !<$$$ 総酸化量: この値が, 酸化すべき粒子の初期質量: (rho_ini*disa*disa*dia) を超えた粒子は酸化しない (発熱しない)
	real(8), pointer :: oxidation_time(:)     !<$$$ 総反応時間: 粒子が水蒸気に触れている時間
	real(8), pointer :: Source_reac(:)     !<$$$ 温度の反応ソース項
	real(8), pointer :: an(:)
	real(8), pointer :: an1(:)
	real(8), pointer :: slr(:)
	type(vector3d), pointer :: pos(:)
	type(vector3d), pointer :: vel(:)
	!ワーク配列
	real(8), pointer :: rwrk(:)
	real(8), pointer :: prs1(:)
	type(vector3d), pointer :: vwrk(:)
	type(vector3d), pointer :: wwrk(:)

	!モジュール手続き
	contains

		subroutine set_particle(nump)

			implicit none

			integer(4) :: nump, i, ierr
			!バッファ
			integer(4), pointer :: ibuf(:)
			real(8), pointer :: rbuf(:)
			type(vector3d), pointer :: vbuf(:)
			
			if(nump.le.0 .or. nump.eq.number_particle) return
			
			if(number_particle.eq.0) then

				!配列の割り付け
				allocate(ityp(nump), stat=ierr)
				allocate(imat(nump), stat=ierr)
				allocate(ibc(nump), stat=ierr)
				allocate(ibct(nump), stat=ierr)
				allocate(isld(nump), stat=ierr)
				allocate(prs(nump), stat=ierr)
				allocate(tmp(nump), stat=ierr)
				allocate(oxidation_mass(nump), stat=ierr)
				allocate(oxidation_time(nump), stat=ierr)
				allocate(Source_reac(nump), stat=ierr)
				allocate(an(nump), stat=ierr)
				allocate(an1(nump), stat=ierr)
				allocate(slr(nump), stat=ierr)
				allocate(pos(nump), stat=ierr)
				allocate(vel(nump), stat=ierr)
				allocate(rho(nump), stat=ierr)
				allocate(rho_ini(nump), stat=ierr)
				allocate(vis(nump), stat=ierr)
				allocate(ald(nump), stat=ierr)
				allocate(sgm(nump), stat=ierr)
				allocate(ang(nump), stat=ierr)
				allocate(cv(nump), stat=ierr)
				allocate(alp(nump), stat=ierr)
				!ワーク配列
				allocate(rwrk(nump), stat=ierr)
				allocate(prs1(nump), stat=ierr)
				allocate(vwrk(nump), stat=ierr)
				allocate(wwrk(nump), stat=ierr)
				
				if(ierr.ne.0) then
					write(6,*) 'ALLOCATE ERROR : set_particle' ; pause ; stop
				endif
				number_particle = nump
				
				do i=1,nump
					ityp(i)= 0
					imat(i)= 0
					ibc(i)= 0
					ibct(i)= 0
					isld(i)= 0
					prs(i)= 0.0d0
					tmp(i)= 0.0d0
					oxidation_mass(i)= 0.0d0
					oxidation_time(i)= 0.0d0
					Source_reac(i)= 0.0d0
					an(i)   = 0.0d0
					an1(i)  = 0.0d0
					slr(i)  = 0.0d0
					rho(i)  = 0.0d0
					rho_ini(i)= 0.0d0
					vis(i)  = 0.0d0
					ald(i)  = 0.0d0
					sgm(i)  = 0.0d0
					ang(i)  = 0.0d0
					cv(i)   = 0.0d0
					alp(i)  = 0.0d0
					pos(i)%x = 0.0d0
					pos(i)%y = 0.0d0
					pos(i)%z = 0.0d0
					vel(i)%x = 0.0d0
					vel(i)%y = 0.0d0
					vel(i)%z = 0.0d0
				enddo

			elseif(number_particle.lt.nump) then

				!配列の拡張
				allocate(ibuf(number_particle), stat=ierr)

				ibuf(1:number_particle) = ityp(1:number_particle)
				deallocate(ityp)
				allocate(ityp(nump), stat=ierr)
				ityp(1:number_particle) = ibuf(1:number_particle)		

				ibuf(1:number_particle) = imat(1:number_particle)
				deallocate(imat)
				allocate(imat(nump), stat=ierr)
				imat(1:number_particle) = ibuf(1:number_particle)

				ibuf(1:number_particle) = ibc(1:number_particle)
				deallocate(ibc)
				allocate(ibc(nump), stat=ierr)
				ibc(1:number_particle) = ibuf(1:number_particle)

				ibuf(1:number_particle) = ibct(1:number_particle)
				deallocate(ibct)
				allocate(ibct(nump), stat=ierr)
				ibct(1:number_particle) = ibuf(1:number_particle)

				ibuf(1:number_particle) = isld(1:number_particle)
				deallocate(isld)
				allocate(isld(nump), stat=ierr)
				isld(1:number_particle) = ibuf(1:number_particle)

				deallocate(ibuf)

				allocate(rbuf(number_particle), stat=ierr)

				rbuf(1:number_particle) = prs(1:number_particle)
				deallocate(prs)
				allocate(prs(nump), stat=ierr)
				prs(1:number_particle) = rbuf(1:number_particle)		

				rbuf(1:number_particle) = tmp(1:number_particle)
				deallocate(tmp)
				allocate(tmp(nump), stat=ierr)
				tmp(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = oxidation_mass(1:number_particle)
				deallocate(oxidation_mass)
				allocate(oxidation_mass(nump), stat=ierr)
				oxidation_mass(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = oxidation_time(1:number_particle)
				deallocate(oxidation_time)
				allocate(oxidation_time(nump), stat=ierr)
				oxidation_time(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = Source_reac(1:number_particle)
				deallocate(Source_reac)
				allocate(Source_reac(nump), stat=ierr)
				Source_reac(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = an(1:number_particle)
				deallocate(an)
				allocate(an(nump), stat=ierr)
				an(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = an1(1:number_particle)
				deallocate(an1)
				allocate(an1(nump), stat=ierr)
				an1(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = slr(1:number_particle)
				deallocate(slr)
				allocate(slr(nump), stat=ierr)
				slr(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = rho(1:number_particle)
				deallocate(rho)
				allocate(rho(nump), stat=ierr)
				rho(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = rho_ini(1:number_particle)
				deallocate(rho_ini)
				allocate(rho_ini(nump), stat=ierr)
				rho_ini(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = vis(1:number_particle)
				deallocate(vis)
				allocate(vis(nump), stat=ierr)
				vis(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = ald(1:number_particle)
				deallocate(ald)
				allocate(ald(nump), stat=ierr)
				ald(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = sgm(1:number_particle)
				deallocate(sgm)
				allocate(sgm(nump), stat=ierr)
				sgm(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = ang(1:number_particle)
				deallocate(ang)
				allocate(ang(nump), stat=ierr)
				ang(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = cv(1:number_particle)
				deallocate(cv)
				allocate(cv(nump), stat=ierr)
				cv(1:number_particle) = rbuf(1:number_particle)

				rbuf(1:number_particle) = alp(1:number_particle)
				deallocate(alp)
				allocate(alp(nump), stat=ierr)
				alp(1:number_particle) = rbuf(1:number_particle)

				!ワーク配列
				deallocate(rwrk)
				allocate(rwrk(nump), stat=ierr)
				deallocate(prs1)
				allocate(prs1(nump), stat=ierr)

				deallocate(rbuf)

				allocate(vbuf(number_particle), stat=ierr)
				vbuf(1:number_particle) = pos(1:number_particle)
				deallocate(pos)
				allocate(pos(nump), stat=ierr)
				pos(1:number_particle) = vbuf(1:number_particle)		
				vbuf(1:number_particle) = vel(1:number_particle)
				deallocate(vel)
				allocate(vel(nump), stat=ierr)
				vel(1:number_particle) = vbuf(1:number_particle)

				!ワーク配列
				deallocate(vwrk)
				allocate(vwrk(nump), stat=ierr)
				deallocate(wwrk)
				allocate(wwrk(nump), stat=ierr)

				deallocate(vbuf)
				
				if(ierr.ne.0) then
					write(6,*) 'ALLOCATE ERROR : set_particle' ; pause ; stop
				endif
				number_particle = nump

			else

				!配列の縮小
				allocate(ibuf(nump), stat=ierr)

				ibuf(1:nump) = ityp(1:nump)
				deallocate(ityp)
				allocate(ityp(nump), stat=ierr)
				ityp(1:nump) = ibuf(1:nump)		

				ibuf(1:nump) = imat(1:nump)
				deallocate(imat)
				allocate(imat(nump), stat=ierr)
				imat(1:nump) = ibuf(1:nump)

				ibuf(1:nump) = ibc(1:nump)
				deallocate(ibc)
				allocate(ibc(nump), stat=ierr)
				ibc(1:nump) = ibuf(1:nump)

				ibuf(1:nump) = ibct(1:nump)
				deallocate(ibct)
				allocate(ibct(nump), stat=ierr)
				ibct(1:nump) = ibuf(1:nump)

				ibuf(1:nump) = isld(1:nump)
				deallocate(isld)
				allocate(isld(nump), stat=ierr)
				isld(1:nump) = ibuf(1:nump)

				deallocate(ibuf)

				allocate(rbuf(nump), stat=ierr)

				rbuf(1:nump) = prs(1:nump)
				deallocate(prs)
				allocate(prs(nump), stat=ierr)
				prs(1:nump) = rbuf(1:nump)		

				rbuf(1:nump) = tmp(1:nump)
				deallocate(tmp)
				allocate(tmp(nump), stat=ierr)
				tmp(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = oxidation_mass(1:nump)
				deallocate(oxidation_mass)
				allocate(oxidation_mass(nump), stat=ierr)
				oxidation_mass(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = oxidation_time(1:nump)
				deallocate(oxidation_time)
				allocate(oxidation_time(nump), stat=ierr)
				oxidation_time(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = Source_reac(1:nump)
				deallocate(Source_reac)
				allocate(Source_reac(nump), stat=ierr)
				Source_reac(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = an(1:nump)
				deallocate(an)
				allocate(an(nump), stat=ierr)
				an(1:nump) = rbuf(1:nump)
				
				rbuf(1:nump) = an1(1:nump)
				deallocate(an1)
				allocate(an1(nump), stat=ierr)
				an1(1:nump) = rbuf(1:nump)
				
				rbuf(1:nump) = slr(1:nump)
				deallocate(slr)
				allocate(slr(nump), stat=ierr)
				slr(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = rho(1:nump)
				deallocate(rho)
				allocate(rho(nump), stat=ierr)
				rho(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = rho_ini(1:nump)
				deallocate(rho_ini)
				allocate(rho_ini(nump), stat=ierr)
				rho_ini(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = vis(1:nump)
				deallocate(vis)
				allocate(vis(nump), stat=ierr)
				vis(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = ald(1:nump)
				deallocate(ald)
				allocate(ald(nump), stat=ierr)
				ald(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = sgm(1:nump)
				deallocate(sgm)
				allocate(sgm(nump), stat=ierr)
				sgm(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = ang(1:nump)
				deallocate(ang)
				allocate(ang(nump), stat=ierr)
				ang(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = cv(1:nump)
				deallocate(cv)
				allocate(cv(nump), stat=ierr)
				cv(1:nump) = rbuf(1:nump)

				rbuf(1:nump) = alp(1:nump)
				deallocate(alp)
				allocate(alp(nump), stat=ierr)
				alp(1:nump) = rbuf(1:nump)

				!ワーク配列
				deallocate(rwrk)
				allocate(rwrk(nump), stat=ierr)
				deallocate(prs1)
				allocate(prs1(nump), stat=ierr)

				deallocate(rbuf)

				allocate(vbuf(nump), stat=ierr)
				vbuf(1:nump) = pos(1:nump)
				deallocate(pos)
				allocate(pos(nump), stat=ierr)
				pos(1:nump) = vbuf(1:nump)		
				vbuf(1:nump) = vel(1:nump)
				deallocate(vel)
				allocate(vel(nump), stat=ierr)
				vel(1:nump) = vbuf(1:nump)

				!ワーク配列
				deallocate(vwrk)
				allocate(vwrk(nump), stat=ierr)
				deallocate(wwrk)
				allocate(wwrk(nump), stat=ierr)

				deallocate(vbuf)
				
				if(ierr.ne.0) then
					write(6,*) 'ALLOCATE ERROR : set_particle' ; pause ; stop
				endif
				number_particle = nump

			endif
			
			return
		end subroutine

end module

!>計算用配列
module MODULE_CALC_ARRAY
	use MODULE_STRUCT

	implicit none !$$$ 2014 2/12 弘中追加

	!近傍リスト長
	integer(4) :: len_neighbor
	!近傍リスト
	integer(4), pointer :: ngh0(:)
	integer(4), pointer :: ngh(:,:)
	real(8), pointer :: dist(:,:)
	!ソルバー用配列
	integer(4), pointer :: icheck(:)
	real(8), pointer :: poiss0(:)
	real(8), pointer :: poiss(:,:)
	real(8), pointer :: source(:)
	real(8), pointer :: p(:)
	real(8), pointer :: q(:)
	real(8), pointer :: r(:)
	real(8), pointer :: s(:)
	real(8), pointer :: dp(:)

	!モジュール手続き
	contains

		subroutine set_calc_array(nump,rer_max,num_dim)
			
			implicit none
			
			include 'parameter.fi'

			integer(4) :: nump, ierr, num_dim
			real(8) :: rer_max
			logical, save :: set_flag = .true.
			
			if(nump.le.0) return
			
			if(set_flag) then

				!近傍リスト長の計算
				if(num_dim.le.2) then
					len_neighbor = dint(1.0d0 * (rer_max * 2.0d0) ** 2)
				else
					len_neighbor = dint(0.7d0 * (rer_max * 2.0d0) ** 3)
				endif

				!配列の割り付け
				allocate(ngh0(nump), stat=ierr)
				allocate(ngh(len_neighbor,nump), stat=ierr)
				allocate(dist(len_neighbor,nump), stat=ierr)
				allocate(icheck(nump), stat=ierr)
				allocate(poiss0(nump), stat=ierr)
				allocate(poiss(len_neighbor,nump), stat=ierr)
				allocate(source(nump), stat=ierr)
				allocate(p(nump), stat=ierr)
				allocate(q(nump), stat=ierr)
				allocate(r(nump), stat=ierr)
				allocate(s(nump), stat=ierr)
				allocate(dp(nump), stat=ierr)

				if(ierr.ne.0) then
					write(6,*) 'ALLOCATE ERROR : set_calc_array' ; pause ; stop
				endif
				set_flag = .false.

			else

				!配列の削除と再割り当て
				deallocate(ngh0)
				deallocate(ngh)
				deallocate(dist)
				deallocate(icheck)
				deallocate(poiss0)
				deallocate(poiss)
				deallocate(source)
				deallocate(p)
				deallocate(q)
				deallocate(r)
				deallocate(s)
				deallocate(dp)

				allocate(ngh0(nump), stat=ierr)
				allocate(ngh(len_neighbor,nump), stat=ierr)
				allocate(dist(len_neighbor,nump), stat=ierr)
				allocate(icheck(nump), stat=ierr)
				allocate(poiss0(nump), stat=ierr)
				allocate(poiss(len_neighbor,nump), stat=ierr)
				allocate(source(nump), stat=ierr)
				allocate(p(nump), stat=ierr)
				allocate(q(nump), stat=ierr)
				allocate(r(nump), stat=ierr)
				allocate(s(nump), stat=ierr)
				allocate(dp(nump), stat=ierr)

				if(ierr.ne.0) then
					write(6,*) 'ALLOCATE ERROR : set_calc_array' ; pause ; stop
				endif

			endif
			
			return
		end subroutine

end module


!>$$$ Edited by S. Hironaka for radiation
module MODULE_RADIATION

	implicit none 
	
	integer:: Nproc_lat, Nproc_x, Nproc_y, Nproc_z									! 各方向のノード分割数
    integer:: Nx, Ny, Nz
    integer:: N_lat, N_lon
    integer:: I_bcd_flg, bcd_x_ini, bcd_x_fin, bcd_y_ini, bcd_y_fin, bcd_z_ini, bcd_z_fin

    integer, pointer:: index_x(:), index_y(:), index_z(:)							! 各粒子の所属セル番号
    integer, pointer:: x_ini(:), x_fin(:), y_ini(:), y_fin(:), z_ini(:), z_fin(:)	! 各粒子の参照セル番号
    real(8), pointer:: x_cell(:), y_cell(:), z_cell(:)								! 各軸方向のセル中心位置

    integer:: picard_max, picard_out, newton_max									! Picard 反復, Newton 反復の最大繰り返し回数
    integer, pointer:: particle_total(:, :, :), ibc_cell(:, :, :)					! 各セルの含有粒子数
    real(8), pointer:: Source_rad_cell(:, :, :), Source_rad_part(:)					! 粒子, セルのソース項

	integer(4), pointer:: cell_state_prev(:, :, :), cell_state(:, :, :)				! セルの状態 (-1: 空, 0: 表面, 1:内部)
    real(8), pointer:: Ir(:, :, :), dIr(:, :, :)									! 輻射輝度, 輻射輝度の変化
    real(8), pointer:: norm_x(:, :, :), norm_y(:, :, :), norm_z(:, :, :)            ! 単位法線ベクトル (セルの稜で定義)
    real(8), pointer:: slr_cell(:, :, :), emi(:, :, :), l_opt(:, :), ads_tot(:, :, :)	! セルの固相率, 射出率, 光学距離, 固体表面の内積の積分値
    real(8), pointer:: rho_cell(:, :, :), cv_cell(:, :, :)							! セルの密度, 比熱
	real(8), pointer:: E_cell(:, :, :), dE_cell(:, :, :)							! Ir の総和, Ir の総和の反復中の差分
	real(8), pointer:: tmp_cell_prev(:, :, :), tmp_cell(:, :, :), dtmp_cell(:, :, :)	! 前の時間ステップのセル温度, セル温度, セル温度の反復中の差分
	
	real(8), pointer:: out(:, :, :)

	real(8):: E_cell_err_max, tmp_cell_err_max, dtmp_cell_err_max					! Ir の総和, セル温度, セル温度の変化量の誤差の閾値
    real(8):: dx, dy, dz, d_lat, d_lon !, tmp_wall, emi_wall						! 空間 & 角度方向刻み幅
	real(8), pointer:: tmp_wall_x_ini(:, :), tmp_wall_x_fin(:, :)					! 壁面温度
	real(8), pointer:: tmp_wall_y_ini(:, :), tmp_wall_y_fin(:, :)					! 壁面温度
	real(8), pointer:: tmp_wall_z_ini(:, :), tmp_wall_z_fin(:, :)					! 壁面温度
	real(8), pointer:: emi_wall_x_ini(:, :), emi_wall_x_fin(:, :)					! 壁面射出率
	real(8), pointer:: emi_wall_y_ini(:, :), emi_wall_y_fin(:, :)					! 壁面射出率
	real(8), pointer:: emi_wall_z_ini(:, :), emi_wall_z_fin(:, :)					! 壁面射出率
    real(8), parameter:: stefan_boltzmann= 5.670373d-08, M_PI= 4.0d0*atan(1.0d0)	! Stefan-Boltzmann 係数と円周率
	real(8), parameter:: DBL_EPSILON= 2.2204460492503131d-16;						! ゼロとみなすべきとても小さい数 (マシンゼロ)
	real(8):: intensity_distribution_factor											! 輻射輝度の温度依存性の角度方向への分配因子 (2D = 2.0 , 3D = M_PI)

	integer(4):: mpi_nbuf;
	! 全体コミュニケータ中でのランク
	integer(4):: mpisize_glob, myrank_glob
	! ローカルコミュニケータ中でのランク
	integer(4):: mpisize, myrank
	integer(4):: rank_x, rank_y, rank_z												! myrank は, 各軸方向に何ランク目のランクか? (0番から数える)
	integer(4):: rank1, rank2, rank3, rank4, rank5, rank6							! 隣接ランク
	real(8), pointer:: mpi_buffer(:)												! MPI 送受信バッファ (配列)
	integer(4), pointer:: mpi_send_int_buffer1(:), mpi_recv_int_buffer1(:)			! x 負方向 MPI 送受信バッファ
	integer(4), pointer:: mpi_send_int_buffer2(:), mpi_recv_int_buffer2(:)			! x 正方向 MPI 送受信バッファ
	integer(4), pointer:: mpi_send_int_buffer3(:), mpi_recv_int_buffer3(:)			! y 負方向 MPI 送受信バッファ
	integer(4), pointer:: mpi_send_int_buffer4(:), mpi_recv_int_buffer4(:)			! y 正方向 MPI 送受信バッファ
	integer(4), pointer:: mpi_send_int_buffer5(:), mpi_recv_int_buffer5(:)			! z 負方向 MPI 送受信バッファ
	integer(4), pointer:: mpi_send_int_buffer6(:), mpi_recv_int_buffer6(:)			! z 正方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer1(:), mpi_recv_dble_buffer1(:)			! x 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer2(:), mpi_recv_dble_buffer2(:)			! x 正方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer3(:), mpi_recv_dble_buffer3(:)			! y 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer4(:), mpi_recv_dble_buffer4(:)			! y 正方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer5(:), mpi_recv_dble_buffer5(:)			! z 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_buffer6(:), mpi_recv_dble_buffer6(:)			! z 正方向 MPI 送受信バッファ
	
	! 法線ベクトル用
	real(8), pointer:: mpi_send_dble_norm1(:), mpi_recv_dble_norm1(:)				! x 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_norm2(:), mpi_recv_dble_norm2(:)				! x 正方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_norm3(:), mpi_recv_dble_norm3(:)				! y 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_norm4(:), mpi_recv_dble_norm4(:)				! y 正方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_norm5(:), mpi_recv_dble_norm5(:)				! z 負方向 MPI 送受信バッファ
	real(8), pointer:: mpi_send_dble_norm6(:), mpi_recv_dble_norm6(:)				! z 正方向 MPI 送受信バッファ
	
	! MPI コミュニケータ
	integer(4):: CommRADIATION

	type local_total
		integer(4), allocatable:: tot(:, :, :)
	end type
	type(local_total), allocatable:: loc_tot(:)


contains
    subroutine set_radiation(ndim, nump, numm, xyz_range)

        use MODULE_STRUCT
        
        implicit none
        ! 引数リスト
        integer:: ndim, nump, numm
        real(8):: lat= 0.0d0, lon= 0.0d0
        type(max_min):: xyz_range(3)
        integer:: p= 0, i= 0, j= 0, k= 0, m= 0, n= 0, l= 0, ierr= 0
        
		allocate(index_x(1:nump), index_y(1:nump), index_z(1:nump), stat= ierr)
		if(ierr.ne.0) then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(x_ini(1:nump), x_fin(1:nump), y_ini(1:nump), y_fin(1:nump), z_ini(1:nump), z_fin(1:nump), stat= ierr)
		if(ierr.ne.0) then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(x_cell(1:Nx), y_cell(1:Ny), z_cell(1:Nz), stat= ierr)
		if(ierr.ne.0) then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif

		allocate(particle_total(1:Nx, 1:Ny, 1:Nz), ibc_cell(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0) then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
!		allocate(Source_rad_cell(1:Nx, 1:Ny, 1:Nz), Source_rad_part(1:nump), stat= ierr)
!		if(ierr.ne.0) then
!		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
!		endif
		allocate(cell_state_prev(1:Nx, 1:Ny, 1:Nz), cell_state(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(Ir(1:Nx, 1:Ny, 1:Nz), dIr(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(norm_x(0:Nx, 0:Ny, 0:Nz), norm_y(0:Nx, 0:Ny, 0:Nz), norm_z(0:Nx, 0:Ny, 0:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(slr_cell(1:Nx, 1:Ny, 1:Nz), emi(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		! d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1 (set_radiation_configuration で 1 足している)
		allocate(l_opt(1:N_lat, 1:N_lon), ads_tot(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(rho_cell(1:Nx, 1:Ny, 1:Nz), cv_cell(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(E_cell(1:Nx, 1:Ny, 1:Nz), dE_cell(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(tmp_cell_prev(1:Nx, 1:Ny, 1:Nz), tmp_cell(1:Nx, 1:Ny, 1:Nz), dtmp_cell(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		
		allocate(tmp_wall_x_ini(1:Ny, 1:Nz), tmp_wall_x_fin(1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(tmp_wall_y_ini(1:Nx, 1:Nz), tmp_wall_y_fin(1:Nx, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(tmp_wall_z_ini(1:Nx, 1:Ny), tmp_wall_z_fin(1:Nx, 1:Ny), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif

		allocate(emi_wall_x_ini(1:Ny, 1:Nz), emi_wall_x_fin(1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(emi_wall_y_ini(1:Nx, 1:Nz), emi_wall_y_fin(1:Nx, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(emi_wall_z_ini(1:Nx, 1:Ny), emi_wall_z_fin(1:Nx, 1:Ny), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif

		allocate(out(1:Nx, 1:Ny, 1:Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif


		! MPI 通信用バッファの確保
		allocate(mpi_buffer(1:Nx*Ny*Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		! 整数
		allocate(mpi_send_int_buffer1(1:Ny*Nz), mpi_recv_int_buffer1(1:Ny*Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_int_buffer2(1:Ny*Nz), mpi_recv_int_buffer2(1:Ny*Nz), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_int_buffer3(1:Nz*Nx), mpi_recv_int_buffer3(1:Nz*Nx), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_int_buffer4(1:Nz*Nx), mpi_recv_int_buffer4(1:Nz*Nx), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_int_buffer5(1:Nx*Ny), mpi_recv_int_buffer5(1:Nx*Ny), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_int_buffer6(1:Nx*Ny), mpi_recv_int_buffer6(1:Nx*Ny), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		! 実数
		allocate(mpi_send_dble_buffer1(1:(Ny+1)*(Nz+1)), mpi_recv_dble_buffer1(1:(Ny+1)*(Nz+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_dble_buffer2(1:(Ny+1)*(Nz+1)), mpi_recv_dble_buffer2(1:(Ny+1)*(Nz+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_dble_buffer3(1:(Nz+1)*(Nx+1)), mpi_recv_dble_buffer3(1:(Nz+1)*(Nx+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_dble_buffer4(1:(Nz+1)*(Nx+1)), mpi_recv_dble_buffer4(1:(Nz+1)*(Nx+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_dble_buffer5(1:(Nx+1)*(Ny+1)), mpi_recv_dble_buffer5(1:(Nx+1)*(Ny+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif
		allocate(mpi_send_dble_buffer6(1:(Nx+1)*(Ny+1)), mpi_recv_dble_buffer6(1:(Nx+1)*(Ny+1)), stat= ierr)
		if(ierr.ne.0)then
		    write(6,*) 'ALLOCATE ERROR : set_radiation' ; pause ; stop
		endif

		do p= 1, nump
			index_x(p)= 0; index_y(p)= 0; index_z(p)= 0; 
			x_ini(p)= 0; x_fin(p)= 0;
			y_ini(p)= 0; y_fin(p)= 0;
			z_ini(p)= 0; z_fin(p)= 0;
!			Source_rad_part(p)= 0.0d0
		enddo
		
		do m= 1, N_lat
			do n= 1, N_lon ! d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1 (set_radiation_configuration で 1 足している)
				l_opt(m, n)= 0.0d0
			enddo
		enddo
		
		l= 1
		do i= 1, Nx
			x_cell(i)= 0.0d0
			do j= 1, Ny
				y_cell(j)= 0.0d0
				do k= 1, Nz
						z_cell(k)= 0.0d0
						
						cell_state_prev(i, j, k)= 0; cell_state(i, j, k)= 0;
						
						particle_total(i, j, k)= 0; ibc_cell(i, j, k)= 0;
!						Source_rad_cell(i, j, k)= 0.0d0; 
						Ir(i, j, k)= 0.0d0; dIr(i, j, k)= 0.0d0; 

						slr_cell(i, j, k)= 0.0d0; emi(i, j, k)= 0.0d0; ads_tot(i, j, k)= 0.0d0
						rho_cell(i, j, k)= 0.0d0; cv_cell(i, j, k)= 0.0d0
						E_cell(i, j, k)= 0.0d0; dE_cell(i, j, k)= 0.0d0;
						tmp_cell_prev(i, j, k)= 0.0d0; tmp_cell(i, j, k)= 0.0d0; dtmp_cell(i, j, k)= 0.0d0
						
						tmp_wall_x_ini(j, k)= 0.0d0; tmp_wall_x_fin(j, k)= 0.0d0;
						tmp_wall_y_ini(i, k)= 0.0d0; tmp_wall_y_fin(i, k)= 0.0d0;
						tmp_wall_z_ini(i, j)= 0.0d0; tmp_wall_z_fin(i, j)= 0.0d0;
						
						emi_wall_x_ini(j, k)= 0.0d0; emi_wall_x_fin(j, k)= 0.0d0;
						emi_wall_y_ini(i, k)= 0.0d0; emi_wall_y_fin(i, k)= 0.0d0;
						emi_wall_z_ini(i, j)= 0.0d0; emi_wall_z_fin(i, j)= 0.0d0;
						
						out(i, j, k)= 0.0d0
!						do m= 1, numm
!								loc_tot(m)%tot(i, j, k)= 0
!						enddo
						
						mpi_buffer(l)= 0.0d0
						l= l+ 1
				enddo
			enddo
		enddo
		
		do i= 0, Nx
			do j= 0, Ny
				do k= 0, Nz
					norm_x(i, j, k)= 0.0d0; norm_y(i, j, k)= 0.0d0; norm_z(i, j, k)= 0.0d0;
				enddo
			enddo
		enddo
		
		do i= 1, Nx
		    x_cell(i)= (dble(i)- 1.5d0)*dx+ xyz_range(1)%min
		enddo
		do j= 1, Ny
		    y_cell(j)= (dble(j)- 1.5d0)*dy+ xyz_range(2)%min
		enddo
		do k= 1, Nz
		    z_cell(k)= (dble(k)- 1.5d0)*dz+ xyz_range(3)%min
		enddo

		lat= -0.5d0*d_lat
		do m= 1, N_lat
			lat= lat+ d_lat

			lon= -d_lon
			do n= 1, N_lon ! d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1 (set_radiation_configuration で 1 足している)
				lon= lon+ d_lon
				if(ndim .eq. 2) lon= M_PI/2.0d0

				l_opt(m, n)= (dabs(cos(lat)*sin(lon))/dx+ dabs(cos(lon))/dy+ dabs(sin(lat)*sin(lon))/dz)
			enddo
		enddo
		
		do l= 1, Ny*Nz
			mpi_send_int_buffer1(l)= 0
			mpi_recv_int_buffer1(l)= 0
			mpi_send_int_buffer2(l)= 0
			mpi_recv_int_buffer2(l)= 0
		enddo
		
		do l= 1, Nz*Nx
			mpi_send_int_buffer3(l)= 0
			mpi_recv_int_buffer3(l)= 0
			mpi_send_int_buffer4(l)= 0
			mpi_recv_int_buffer4(l)= 0
		enddo
		
		do l= 1, Nx*Ny
			mpi_send_int_buffer5(l)= 0
			mpi_recv_int_buffer5(l)= 0
			mpi_send_int_buffer6(l)= 0
			mpi_recv_int_buffer6(l)= 0
		enddo

		do l= 1, (Ny+1)*(Nz+1)
			mpi_send_dble_buffer1(l)= 0.0d0
			mpi_recv_dble_buffer1(l)= 0.0d0
			mpi_send_dble_buffer2(l)= 0.0d0
			mpi_recv_dble_buffer2(l)= 0.0d0
		enddo
		
		do l= 1, (Nz+1)*(Nx+1)
			mpi_send_dble_buffer3(l)= 0.0d0
			mpi_recv_dble_buffer3(l)= 0.0d0
			mpi_send_dble_buffer4(l)= 0.0d0
			mpi_recv_dble_buffer4(l)= 0.0d0
		enddo
		
		do l= 1, (Nx+1)*(Ny+1)
			mpi_send_dble_buffer5(l)= 0.0d0
			mpi_recv_dble_buffer5(l)= 0.0d0
			mpi_send_dble_buffer6(l)= 0.0d0
			mpi_recv_dble_buffer6(l)= 0.0d0
		enddo
	
		! 輻射輝度の角度方向分配因子
		if(ndim .eq. 2) then
			intensity_distribution_factor= 2.0d0
		else
			intensity_distribution_factor= M_PI
		endif

		! 境界条件の設定
		bcd_x_ini= int(I_bcd_flg/100000)

		I_bcd_flg= I_bcd_flg- 100000*int(I_bcd_flg/100000)
		bcd_x_fin= int(I_bcd_flg/10000)

		I_bcd_flg= I_bcd_flg- 10000*int(I_bcd_flg/10000)
		bcd_y_ini= int(I_bcd_flg/1000)

		I_bcd_flg= I_bcd_flg- 1000*int(I_bcd_flg/1000)
		bcd_y_fin= int(I_bcd_flg/100)
		
		! 二次元の場合は、y 方向の境界を強制的に周期境界とする
		if(ndim .eq. 2) then
			bcd_y_ini= 3
			bcd_y_fin= 3
		endif

		I_bcd_flg= I_bcd_flg- 100*int(I_bcd_flg/100)
		bcd_z_ini= int(I_bcd_flg/10)

		I_bcd_flg= I_bcd_flg- 10*int(I_bcd_flg/10)
		bcd_z_fin= I_bcd_flg
		
		! 周期境界の一貫性 (どちらか一方の端が周期なら、両端が周期であること) を確認する
		! x 方向
		if(bcd_x_ini .eq. 3) then
			if(bcd_x_fin .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif
		
		if(bcd_x_fin .eq. 3) then
			if(bcd_x_ini .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif


		! y 方向
		if(bcd_y_ini .eq. 3) then
			if(bcd_y_fin .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif
		
		if(bcd_y_fin .eq. 3) then
			if(bcd_y_ini .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif
		

		! z 方向
		if(bcd_z_ini .eq. 3) then
			if(bcd_z_fin .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif

		if(bcd_z_fin .eq. 3) then
			if(bcd_z_ini .ne. 3) then
				write(6, '(a)') "!!! Invalid periodic boundary condition: !!!"
				write(6, '((a3), 2(I3))') "x: ", bcd_x_ini, bcd_x_fin
				write(6, '((a3), 2(I3))') "y: ", bcd_y_ini, bcd_y_fin
				write(6, '((a3), 2(I3))') "z: ", bcd_z_ini, bcd_z_fin
				call MPI_Finalize(ierr)
				stop
			endif
		endif
	end subroutine
    
end module

!>固体リストと接触点配列
module MODULE_SOLID_ARRAY
	use MODULE_STRUCT

	!固体アレイ数
	integer(4) :: num_sld_ary !可変を想定する必要がある(孤立固体=紛体はアレイにカウントしない)
	!固体リスト
	integer(4), pointer :: idx_sld(:) !サイズはnum_sld_ary+1であるが粒子数より大きくなることはあり得ない
	integer(4), pointer :: lst_sld(:) !サイズは粒子数
	!固体アレイ配列(とことんケチる->作用力なども計算時にローカル変数で処理)
	real(8), allocatable :: sld_pos(:,:)    !重心
	real(8), allocatable :: sld_quat(:,:)   !姿勢(4元数s,x,y,z)
	real(8), allocatable :: sld_moi0(:,:,:) !慣性テンソルの逆行列の初期値
	real(8), allocatable :: sld_moi1(:,:,:) !慣性テンソルの逆行列
	
	!接触点配列(サイズは(3*L0)^ndim)
	integer(4), allocatable :: id_cont(:,:) !接触点(相手番号を格納)->毎ステップ新規分離判定により詰める
	real(8), allocatable :: dis_tan(:) !接線変位ベクトル
end module
