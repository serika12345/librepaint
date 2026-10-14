kis_add_tests(
    kis_kra_loader_test.cpp
    LINK_LIBRARIES kritaapplicationui kritalibkra kritaimpexkra kritatestsdk
    )


kis_add_test(
    kis_kra_saver_test.cpp
    LINK_LIBRARIES kritaapplicationui kritalibkra kritatransformmaskstubs
    )

add_dependencies(kis_kra_loader_test
    kritakraimport kritacolorgenerator kritapatterngenerator
    kritapixelizefilter krita_shape_paths krita_shape_image)
add_dependencies(kis_kra_saver_test
    kritakraimport kritakraexport kritacolorgenerator kritapatterngenerator
    kritapixelizefilter krita_shape_paths krita_shape_image)
