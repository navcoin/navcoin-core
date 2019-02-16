CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

unset(Qt5Enables)
foreach(comp Widgets Core Gui Network LinguistTools DBus PrintSupport)
  find_package(Qt5${comp})
  set(Qt5Enables "${Qt5Enables};Qt5${comp}_FOUND")
  if(Qt5${comp}_FOUND)
    MESSAGE(STATUS "FOUND QT5${comp}")
    include_directories(${include_directories} ${Qt5${comp}_INCLUDE_DIRS} ${QT_USE_FILE})
    add_definitions(${Qt5${comp}_DEFINITIONS})
    list(APPEND Qt5_LIBRARIES ${Qt5${comp}_LIBRARIES})
  else()
    MESSAGE(WARNING "Cant find Qt5${comp}")
  endif()
endforeach(comp)


find_package(Qt5DBus)
if (Qt5DBus_FOUND)
  include_directories(${include_directories} "/usr/local/opt/qt5/include/QtDBus/")
  add_compile_options("-DUSE_DBUS")
endif()

# Why isn't this done automatically??
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_AUTOMOC_MOC_OPTIONS "-DQ_OS_MAC")
endif()


file(GLOB LOCAL_QT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
source_group("QtHeaders" FILES ${LOCAL_QT_HEADERS})

set(GUI_FORMS
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/addressbookpage.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/askpassphrasedialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/cfund_voting.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/coincontroldialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/debugwindow.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/editaddressdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/helpmessagedialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/intro.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/openuridialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/optionsdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/overviewpage.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/receivecoinsdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/receiverequestdialog.ui
#  ${CMAKE_CURRENT_SOURCE_DIR}/forms/rpcconsole.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/sendcoinsdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/sendcoinsentry.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/signverifymessagedialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/forms/transactiondescdialog.ui
  )
  
QT5_WRAP_UI( UI_HEADERS ${GUI_FORMS})

include_directories(${include_directories} ${CMAKE_CURRENT_BINARY_DIR})


SET(QT_FILES
	../script/standard.cpp
  ../rpc/client.cpp
	devaultaddressvalidator.cpp
	devaultamountfield.cpp
	devaultgui.cpp
	devaultunits.cpp
	devaultstrings.cpp
	clientmodel.cpp
	csvmodelwriter.cpp
	guiutil.cpp
	intro.cpp

  ./forms/cfund_voting.cpp
  skinize.cpp
  getaddresstoreceive.cpp
  coldstakingwizard.cpp
  
	notificator.cpp
	optionsdialog.cpp
	optionsmodel.cpp
	peertablemodel.cpp
	platformstyle.cpp
	qvalidatedlineedit.cpp
	qvaluecombobox.cpp
	networkstyle.cpp
	rpcconsole.cpp
	bantablemodel.cpp
	splashscreen.cpp
	trafficgraphwidget.cpp
	utilitydialog.cpp
	winshutdownmonitor.cpp
	addressbookpage.cpp
	addresstablemodel.cpp
	askpassphrasedialog.cpp
	coincontroldialog.cpp
	coincontroltreewidget.cpp
	editaddressdialog.cpp
	openuridialog.cpp
	overviewpage.cpp
	receivecoinsdialog.cpp
	receiverequestdialog.cpp
	recentrequeststablemodel.cpp
	sendcoinsdialog.cpp
	sendcoinsentry.cpp
	signverifymessagedialog.cpp
	transactiondesc.cpp
	transactiondescdialog.cpp
	transactionfilterproxy.cpp
	transactionrecord.cpp
	transactiontablemodel.cpp
	transactionview.cpp
	walletframe.cpp
	walletmodel.cpp
	walletmodeltransaction.cpp
	walletview.cpp

	)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_library(mac_stuff
	macdockiconhandler.mm
	macnotificationhandler.mm
	)
set_property(TARGET mac_stuff PROPERTY CXX_STANDARD 11)
endif()

ADD_LIBRARY(qt_stuff ${QT_FILES} ${UI_HEADERS})

set_property(TARGET qt_stuff PROPERTY CXX_STANDARD 11)

file(GLOB QT_TRANSLATIONS_FILES locale/*.ts)

SET(QT_TRANSLATIONS_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_af.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_af_ZA.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ar.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_be_BY.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_bg.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_bg_BG.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ca.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ca@valencia.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ca_ES.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_cs.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_cs_CZ.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_cy.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_da.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_de.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_el.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_el_GR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_en.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_en_GB.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_eo.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_AR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_CL.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_CO.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_DO.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_ES.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_MX.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_UY.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_es_VE.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_et.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_eu_ES.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fa.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fa_IR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fi.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fr.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fr_CA.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_fr_FR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_gl.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_he.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_hi_IN.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_hr.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_hu.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_id_ID.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_it.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_it_IT.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ja.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ka.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_kk_KZ.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ko_KR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ku_IQ.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ky.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_la.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_lt.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_lv_LV.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_mk_MK.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_mn.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ms_MY.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_nb.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_nl.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_pam.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_pl.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_pt_BR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_pt_PT.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ro.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ro_RO.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ru.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ru_RU.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sk.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sl_SI.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sq.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sr.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sr@latin.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_sv.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ta.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_th_TH.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_tr.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_tr_TR.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_uk.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_ur_PK.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_uz@Cyrl.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_vi.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_vi_VN.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_zh.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_zh_CN.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_zh_HK.ts
	${CMAKE_CURRENT_SOURCE_DIR}/locale/devault_zh_TW.ts

	)

get_filename_component(QM_FILES_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/locale" ABSOLUTE)
set_source_files_properties(${QT_TRANSLATIONS_FILES} PROPERTIES OUTPUT_LOCATION "${QM_FILES_BINARY_DIR}")

QT5_ADD_TRANSLATION( QM ${QT_TRANSLATIONS_FILES} )
add_custom_target (translations_target DEPENDS ${QM}) 

QT5_ADD_RESOURCES(QRC_RESOURCE devault.qrc)
QT5_ADD_RESOURCES(QRC_LOCALE_RESOURCE devault_locale.qrc)

add_executable(devault-qt devault.cpp ${QM} ${QRC_RESOURCE} ${QRC_LOCALE_RESOURCE} ${GUI_FORM_HEADERS})
add_dependencies(devault-qt translations_target)
#set_property(TARGET devault-qt PROPERTY CXX_STANDARD 11)
#set_source_files_properties(devault.cpp PROPERTIES CMAKE_CXX_FLAGS "-std=c++11")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  TARGET_LINK_LIBRARIES(devault-qt coin qt_stuff mac_stuff ${BDBXX_LIBRARY} ${BDB_LIBRARY} secp256k1 zerocoin unbound curl leveldb ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${ANL} qrencode miniupnpc zmq gmp event event_pthreads pthread "-framework Cocoa")
else()
  TARGET_LINK_LIBRARIES(devault-qt coin qt_stuff ${BDBXX_LIBRARY} ${BDB_LIBRARY} secp256k1 leveldb zerocoin unbound curl ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${ANL} qrencode miniupnpc zmq gmp event event_pthreads pthread)
endif()
QT5_USE_Modules(devault-qt Gui)
QT5_USE_Modules(devault-qt Core)
QT5_USE_Modules(devault-qt Widgets)
QT5_USE_Modules(devault-qt Test)
QT5_USE_Modules(devault-qt PrintSupport)
QT5_USE_Modules(devault-qt Network)
if (Qt5DBus_FOUND)
  QT5_USE_Modules(devault-qt DBus)
endif()


#INSTALL(TARGETS devault-qt DESTINATION bin)
