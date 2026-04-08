subroutine output_result_data(ndim, nump, output_id, it, dt, xyz_range, out_binary_buffer, output_path)

	use MODULE_STRUCT
	use MODULE_RADIATION
!	use MODULE_PARTICLE
	
	implicit none
!	引数リスト    
	integer:: ndim, nump, output_id, it
	real(8):: dt
	character(*):: output_path
	type(max_min):: xyz_range(3)

    integer:: i= 0, j= 0, k= 0, l= 0
    ! x, y, z 方向の始点・終点, 出力データの長さ
	integer:: i_ini, i_fin, k_ini, k_fin, j_ini, j_fin, len_data
	real(8):: tmp_max= 0.0d0, tmp_min= 0.0d0, tmp_ave= 0.0d0
	
	real(8):: norm_x_cell= 0.0d0, norm_y_cell= 0.0d0, norm_z_cell= 0.0d0, norm_cell= 0.0d0
        
	character(255):: file_name= "\0", out_binary_buffer_str= "\0"
    type(bindata1):: sepr
    type(bindata4):: out_binary_buffer(1:3*(Nx+1)*(Ny+1)*(Nz+1))


    write(file_name, '(I6)') it
    do i= 1, 6
        if(file_name(i:i) .eq. ' ') file_name(i:i)= '0'
	enddo
	write(file_name, '((I4), (a))') myrank, '_'//trim(file_name)
    do i= 1, 4
        if(file_name(i:i) .eq. ' ') file_name(i:i)= '0'
	enddo
    file_name= trim(output_path)//'result_'//trim(file_name)//'.vtk'


    open(output_id, file= trim(file_name), status= 'replace', form= 'binary')

	
    sepr%int= Z'0A'
    write(output_id) '# vtk DataFile Version 2.0'//sepr%cha
    write(output_id) 'Radiation Program result'//sepr%cha
    write(output_id) 'BINARY'//sepr%cha
    write(output_id) 'DATASET STRUCTURED_POINTS'//sepr%cha


	i_ini= 2
	i_fin= Nx- 1
	if(rank1 .eq. -1) then
		i_ini= 1
	endif
	if(rank2 .eq. -1) then
		i_fin= Nx
	endif

	if(ndim .eq. 2) then
        j_ini= 2
        j_fin= 2
    else
        j_ini= 2
        j_fin= Ny- 1
		if(rank3 .eq. -1) then
			j_ini= 1
		endif
		if(rank4 .eq. -1) then
			j_fin= Ny
		endif
	endif

	k_ini= 2
	k_fin= Nz- 1
	if(rank5 .eq. -1) then
		k_ini= 1
	endif
	if(rank6 .eq. -1) then
		k_fin= Nz
	endif

	!i_ini= 1
	!i_fin= Nx
	!k_ini= 1
	!k_fin= Nz

	if(ndim .eq. 2) then
        len_data= (i_fin- i_ini+ 1)*(k_fin- k_ini+ 1)
        write(out_binary_buffer_str, '((a11), 3(I5), (a1))') 'DIMENSIONS ', (i_fin- i_ini+ 2),  1, (k_fin- k_ini+ 2), sepr%cha
        write(output_id) trim(out_binary_buffer_str)
    else
        len_data= (i_fin- i_ini+ 1)*(j_fin- j_ini+ 1)*(k_fin- k_ini+ 1)
        write(out_binary_buffer_str, '((a11), 3(I5), (a1))') 'DIMENSIONS ', (i_fin- i_ini+ 2), (j_fin- j_ini+ 2), (k_fin- k_ini+ 2), sepr%cha
        write(output_id) trim(out_binary_buffer_str)
	endif


    write(out_binary_buffer_str, '((a7), 3(E12.3), (a1))') 'ORIGIN ', xyz_range(1)%min+ rank_x*(xyz_range(1)%max- xyz_range(1)%min+ dx)- dx &
																	, xyz_range(2)%min+ rank_y*(xyz_range(2)%max- xyz_range(2)%min+ dy)- dy &
																	, xyz_range(3)%min+ rank_z*(xyz_range(3)%max- xyz_range(3)%min+ dz)- dz, sepr%cha
    write(output_id) trim(out_binary_buffer_str)

    write(out_binary_buffer_str, '((a8), 3(E12.3), (a1))') 'SPACING ', dx, dy, dz, sepr%cha
    write(output_id) trim(out_binary_buffer_str)

    write(out_binary_buffer_str, '((a10), (I8), (a1))') 'CELL_DATA ', len_data, sepr%cha
    write(output_id) trim(out_binary_buffer_str)





    ! ランク
    write(out_binary_buffer_str, '(a)') 'SCALARS MPI_RANK int'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%int= myrank
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 温度
    write(out_binary_buffer_str, '(a)') 'SCALARS Temperature[K] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(tmp_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 温度差分
    write(out_binary_buffer_str, '(a)') 'SCALARS Temperature_diff[K] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(dtmp_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 吸収係数積分値
    write(out_binary_buffer_str, '(a)') 'SCALARS ads_total[-] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(ads_tot(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 比熱
    write(out_binary_buffer_str, '(a)') 'SCALARS CV[J/kg/K] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(cv_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 密度
    write(out_binary_buffer_str, '(a)') 'SCALARS Density[kg/m3] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(rho_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 固相割合
    write(out_binary_buffer_str, '(a)') 'SCALARS Solid_fraction[-] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(slr_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 輻射エネルギー密度 ( I の角度方向積分値)
    write(out_binary_buffer_str, '(a)') 'SCALARS Energy_Density[W/m3] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(E_cell(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 放出輻射エネルギー密度 ( I の角度方向積分値)
	write(out_binary_buffer_str, '(a)') 'SCALARS Energy_Density(out)[W/m2] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
	write(output_id) trim(out_binary_buffer_str)
	l= 1
	do k= k_ini, k_fin
		do j= j_ini, j_fin
			do i= i_ini, i_fin
				out_binary_buffer(l)%flt= sngl(ads_tot(i, j, k)*stefan_boltzmann*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)/intensity_distribution_factor)
				l= l+ 1
			enddo
		enddo
	enddo
	call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! 射出率
    write(out_binary_buffer_str, '(a)') 'SCALARS Emissivity[-] float'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%flt= sngl(emi(i, j, k))
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
    ! セルの状態
    write(out_binary_buffer_str, '(a)') 'SCALARS Cell_State[-] int'//sepr%cha//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
                out_binary_buffer(l)%int= cell_state(i, j, k)
                l= l+ 1
            enddo
        enddo
    enddo
    call convert_endian(output_id, len_data, 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)
 
	! 法線ベクトル
    write(out_binary_buffer_str, '(a)') 'VECTORS Normal[-] float'//sepr%cha  !//'LOOKUP_TABLE default'//sepr%cha
    write(output_id) trim(out_binary_buffer_str)
    l= 1
    do k= k_ini, k_fin
        do j= j_ini, j_fin
            do i= i_ini, i_fin
				norm_x_cell= 0.125d0*( norm_x(i-1, j-1, k-1)+ norm_x(i-1, j-1, k)+ norm_x(i-1, j, k-1)+ norm_x(i-1, j, k) &
								     + norm_x(i,   j-1, k-1)+ norm_x(i,   j-1, k)+ norm_x(i,   j, k-1)+ norm_x(i,   j, k) )
				norm_y_cell= 0.125d0*( norm_y(i-1, j-1, k-1)+ norm_y(i-1, j-1, k)+ norm_y(i-1, j, k-1)+ norm_y(i-1, j, k) &
								     + norm_y(i,   j-1, k-1)+ norm_y(i,   j-1, k)+ norm_y(i,   j, k-1)+ norm_y(i,   j, k) )
				norm_z_cell= 0.125d0*( norm_z(i-1, j-1, k-1)+ norm_z(i-1, j-1, k)+ norm_z(i-1, j, k-1)+ norm_z(i-1, j, k) &
								     + norm_z(i,   j-1, k-1)+ norm_z(i,   j-1, k)+ norm_z(i,   j, k-1)+ norm_z(i,   j, k) )
				
				norm_cell= sqrt(norm_x_cell*norm_x_cell+ norm_y_cell*norm_y_cell+ norm_z_cell*norm_z_cell)
				
				if(norm_cell .gt. DBL_EPSILON) then
					norm_x_cell= norm_x_cell/norm_cell
					norm_y_cell= norm_y_cell/norm_cell
					norm_z_cell= norm_z_cell/norm_cell
				else
					norm_x_cell= 0.0d0
					norm_y_cell= 0.0d0
					norm_z_cell= 0.0d0
				endif
    
                out_binary_buffer(l)%flt= sngl(norm_x_cell)
				out_binary_buffer(l+1)%flt= sngl(norm_y_cell)
				out_binary_buffer(l+2)%flt= sngl(norm_z_cell)
                l= l+ 3
            enddo
        enddo
    enddo
    call convert_endian(output_id, 3*(Nx+1)*(Ny+1)*(Nz+1), 3*(Nx+1)*(Ny+1)*(Nz+1), out_binary_buffer)

    close(output_id)


end


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!   バイナリデータの出力とエンディアン変換
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
subroutine convert_endian(output_id, buff_len, buff_len_max, buff)

    use MODULE_STRUCT

    implicit none
    
    ! 引数リスト
    integer :: output_id, buff_len, buff_len_max
    type(bindata4) :: buff(1:buff_len_max)
 
    character(1):: b_swp
    integer(4) :: i
 
	!以下の並列化処理はifortの自動並列化機能を有効にしているとおかしな結果となるので要注意(原則自動並列化は使用しない!!)
	!$omp parallel do private(b_swp)
	do i = 1, buff_len
		b_swp = buff(i)%cha(1:1)
		buff(i)%cha(1:1) = buff(i)%cha(4:4)
        buff(i)%cha(4:4)= b_swp
 
        b_swp = buff(i)%cha(2:2)
		buff(i)%cha(2:2) = buff(i)%cha(3:3)
        buff(i)%cha(3:3)= b_swp
	enddo
	!$omp end parallel do
 
    write(output_id) buff(1:buff_len)

    return
end
