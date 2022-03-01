# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

if(CONFIG_LCZ_MCUMGR_CMD_FS_MGMT)
zephyr_include_directories(lcz_fs_mgmt/include)

zephyr_sources(
    lcz_fs_mgmt/port/zephyr/src/zephyr_lcz_fs_mgmt.c
    lcz_fs_mgmt/src/lcz_fs_mgmt.c
)
endif()

if(CONFIG_LCZ_MCUMGR_CMD_OS_MGMT)
zephyr_include_directories(lcz_os_mgmt/include)

zephyr_sources(
    lcz_os_mgmt/port/zephyr/src/zephyr_lcz_os_mgmt.c
    lcz_os_mgmt/src/lcz_os_mgmt.c
    lcz_os_mgmt/src/stubs.c
)
endif()

if(CONFIG_LCZ_MCUMGR_CMD_IMG_MGMT)
zephyr_include_directories(lcz_img_mgmt/include)

zephyr_sources(
    lcz_img_mgmt/src/zephyr_img_mgmt.c
    lcz_img_mgmt/src/zephyr_img_mgmt_log.c
    lcz_img_mgmt/src/img_mgmt.c
    lcz_img_mgmt/src/img_mgmt_state.c
    lcz_img_mgmt/src/img_mgmt_util.c
)
endif()
