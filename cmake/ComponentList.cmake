#
# Generates simple runtime iterable arrays from enum representing types.
# 
# Takes:

#
function(ComponentList_generate infile outfile)
    # read file, find the enum, match the contents between {}
    file(READ ${infile} whole_file)
    string(REGEX MATCH "enum[ \t\r\n]+class[ \t\r\n]+Components[ \t\r\n]*{([^}]*)}" _ ${whole_file})

    # test to make sure we found contents in the enum
    if(NOT CMAKE_MATCH_1)
        message(FATAL_ERROR "Could not find Components enum")
        return()
    endif()

    # strip ALL whitespace
    string(REGEX REPLACE "[ \t\r\n]" "" items ${CMAKE_MATCH_1})
    # convert to list
    string(REPLACE "," ";" items ${items})
    
    # message(STATUS "++++++++++++++++++++++")
    # message(STATUS "${items}")
    # message(STATUS "++++++++++++++++++++++")
    
    # look at the contents and generate list of types and ids
    set(type_id_pairs)
    foreach(item ${items})
        # skip if empty
        if(item STREQUAL "")
            continue()
        endif()
        # match the "Type = 1" sequence
        string(REGEX MATCH "([A-Za-z_]+[A-Za-z0-9_]*)=([0-9]+)" _ ${item})
        # show warning if matches are not found at all
        if(NOT CMAKE_MATCH_1 OR NOT CMAKE_MATCH_2)
            message(WARNING "Error parsing component in Components enum (${item})")
            continue()
        endif()
        list(APPEND type_id_pairs "${CMAKE_MATCH_1},${CMAKE_MATCH_2}")
    endforeach()

    # message(STATUS "TYPES=${type_id_pairs}")

    set(ComponentList_message)
    string(CONCAT ComponentList_message
        "//\n"
        "// Generated by CMAKE. DO NOT EDIT.\n"
        "// Edit Components enum in ${infile}\n"
        "//\n"
    )

    list(LENGTH type_id_pairs ComponentList_typeCount)

    set(count 0)
    foreach(item ${type_id_pairs})
        string(REPLACE "," ";" item ${item})
        list(GET item 0 type)
        list(GET item 1 id)
        set(if_name "if (strcmp(name, allComponents[${count}]) == 0)")
        set(if_index "if (index == ${count})")
        set(endl   "\n    ")

        string(APPEND ComponentList_includes
            "#include \"${type}.h\"\n"
        )

        string(APPEND ComponentList_typeListAsStrings
            "\"${type}\",${endl}"
        )

        string(APPEND ComponentList_fnComponentId
            "${if_name} return ${id};${endl}"
        )

        string(APPEND ComponentList_fnComponentIndex
            "${if_name} return ${count};${endl}"
        )

        string(APPEND ComponentList_fnEmplaceComponentByName
            "${if_name} r.emplace<${type}>(e);${endl}"
        )

        string(APPEND ComponentList_fnEmplaceComponentByIndex
            "${if_index} r.emplace<${type}>(e);${endl}"
        )

        math(EXPR count "${count} + 1")
    endforeach()

    # set(root_dir ${CMAKE_CURRENT_SOURCE_DIR})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${infile}" "${CMAKE_CURRENT_SOURCE_DIR}/${outfile}")

    add_custom_target(
        ComponentList_target
        DEPENDS ${outfile}
    )
endfunction()