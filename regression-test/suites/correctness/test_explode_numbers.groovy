// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

suite("test_explode_numbers") {
    sql 'set enable_nereids_planner=true'
    qt_select1 """
        select e1 from (select 1 k1) as t lateral view explode_numbers(5) tmp1 as e1 order by e1;
    """
    qt_select2 """
        select e1 from (select 1 k1) as t lateral view explode_numbers(0) tmp1 as e1;
    """
    qt_select3 """
        select e1 from (select 1 k1) as t lateral view explode_numbers(null) tmp1 as e1;
    """    
    qt_select4 """
       select e1 from (select 1 k1) as t lateral view explode_numbers(-5) tmp1 as e1;
    """    
}