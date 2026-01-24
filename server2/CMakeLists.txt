add_executable(server2 main.cpp mpeg.cpp)

if(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  find_library(CFNETWORK_FRAMEWORK CFNetwork REQUIRED)

  target_link_libraries(server2 PRIVATE
    ${COREFOUNDATION_FRAMEWORK}
    ${CFNETWORK_FRAMEWORK}
  )
endif()

find_package(httplib CONFIG REQUIRED)
target_link_libraries(server2 PRIVATE httplib::httplib)

# -----------------------------
# Vite（web）ビルドを組み込む
# -----------------------------
set(WEB_DIR      "${CMAKE_SOURCE_DIR}/web")
set(WEB_DIST_DIR "${WEB_DIR}/dist")

find_program(NPM_EXECUTABLE npm REQUIRED)

# （任意）npm install が毎回走るのが嫌なら後で最適化できます
add_custom_target(vite_build
  COMMAND "${NPM_EXECUTABLE}" install
  COMMAND "${NPM_EXECUTABLE}" run build
  WORKING_DIRECTORY "${WEB_DIR}"
  COMMENT "Building frontend (Vite)"
  VERBATIM
)

# server2 をビルドする前に Vite build を必ず実行
add_dependencies(server2 vite_build)

# -----------------------------
# 配布（ステージング）
# -----------------------------
# ここを好きな出力先に変える（例: ${CMAKE_BINARY_DIR}/dist）
set(STAGE_DIR "${CMAKE_SOURCE_DIR}/dist")

add_custom_command(TARGET server2 POST_BUILD
  # dist を作る
  COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGE_DIR}"

  # 実行ファイルを dist/ へ
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:server2>" "${STAGE_DIR}/"

  # public は一旦消してから作り直す（copy_directory失敗対策）
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${STAGE_DIR}/public"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGE_DIR}/public"

  # Vite のビルド成果物（web/dist）を dist/public へ
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${WEB_DIST_DIR}" "${STAGE_DIR}/public"

  COMMENT "Staging server2 + web/dist -> ${STAGE_DIR}"
  VERBATIM
)