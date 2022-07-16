function(FileUtils_makeClearCache
    output_dir)
    set(output_file "${output_dir}/clearcache")
    if (NOT EXISTS "${output_file}")
        set(temp_dir "${output_dir}/temp")
        set(temp_file "${temp_dir}/clearcache")
        message(STATUS "CREATING UTILITY FILE ${output_file}")
        
        file(WRITE "${temp_file}" "find . -name CMakeCache.txt -delete\nrm -rf CMakeFiles\n")
        
        file(COPY 
            "${temp_file}" 
            DESTINATION "${output_dir}"
            FILE_PERMISSIONS 
                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
        )

        file(GLOB RESULT "${temp_dir}")
        list(LENGTH RESULT RES_LEN)
        message(STATUS "RES_LEN = ${RES_LEN}")
        if(RES_LEN EQUAL 1)
            # temp dir only has our file, delete whole temp dir
            file(REMOVE_RECURSE "${temp_dir}")
        elseif()
            # temp dir existed before, just delete our file
            file(REMOVE "${temp_file}")
        endif()

    endif()
endfunction()
