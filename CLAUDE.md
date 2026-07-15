# crud

C++20 Visual Studio 프로젝트. `crud/` 폴더에 다음 두 라이브러리가 포함되어 있습니다. 외부 의존성 없이 표준 라이브러리만 사용합니다.

- JSON 파싱/저장 라이브러리 (`json::Value`)
- 퀵정렬 라이브러리 (`sortlib::QuickSort`)

## 프로젝트 목표

데이터를 JSON 파일로 관리하는 CRUD(Create, Read, Update, Delete) 콘솔 애플리케이션을 개발한다.

- 레코드를 생성/조회/수정/삭제하는 콘솔 명령/메뉴를 제공한다.
- 모든 데이터는 `json::Value`(`crud/Json.h`, `crud/Json.cpp`)를 통해 JSON 파일로 저장하고 불러온다.
- 목록 조회 시 정렬이 필요하면 `sortlib::QuickSort`(`crud/QuickSort.h`)를 재사용한다.
- 새 기능은 gtest/gmock(`crud/*Tests.cpp`)으로 테스트 시나리오를 함께 작성한다.

## 에이전트 (`agents/`)

이 프로젝트에서 작업할 때 참고할 전문 에이전트 역할 정의가 `agents/` 폴더에 있습니다.

- [`agents/code-expert.md`](agents/code-expert.md) — 코드 전문가: CRUD 콘솔 애플리케이션 및 라이브러리의 프로덕션 코드 설계/구현 담당
- [`agents/test-expert.md`](agents/test-expert.md) — 테스트 전문가: gtest/gmock 기반 테스트 시나리오 설계/작성 담당

## 빌드

- 솔루션: `crud.slnx`
- 프로젝트: `crud/crud.vcxproj` (ConfigurationType: Application, C++20, Win32/x64 x Debug/Release)
- Visual Studio에서 열어 빌드하거나 `MSBuild crud/crud.vcxproj /p:Configuration=Debug /p:Platform=x64` 로 빌드

## JSON 라이브러리 (`crud/Json.h`, `crud/Json.cpp`)

`json` 네임스페이스에 `json::Value` 클래스로 JSON 값(null, boolean, number, string, array, object)을 표현합니다.

### 파싱

```cpp
#include "Json.h"

json::Value v = json::Value::Parse(R"({"name":"crud","version":1})"); // 문자열에서 파싱
json::Value fromFile = json::Value::ParseFile("data.json");           // 파일에서 파싱
```

- 잘못된 JSON은 `json::ParseException` (line/column 정보 포함, `std::runtime_error` 파생)을 던집니다.
- 파일을 열 수 없으면 `std::runtime_error`를 던집니다.
- `\uXXXX` 유니코드 이스케이프(서로게이트 쌍 포함)를 UTF-8로 디코딩합니다.

### 값 생성 및 접근

```cpp
json::Value root = json::Value::MakeObject();
root.Set("name", "crud");
root.Set("version", 1);
root.Set("active", true);

json::Value tags = json::Value::MakeArray();
tags.PushBack("json");
tags.PushBack("library");
root.Set("tags", tags);

std::string name = root["name"].AsString();
double version = root["version"].AsNumber();
bool active = root["active"].AsBool();
bool hasTags = root.Contains("tags");
```

- `AsBool()`, `AsNumber()`, `AsString()`, `AsArray()`, `AsObject()`는 타입이 다르면 `std::runtime_error`를 던집니다.
- `operator[](const std::string&)`는 object에 없는 키를 쓰기 목적으로 접근하면 자동으로 멤버를 추가합니다(읽기 전용 `const` 버전은 없으면 예외).
- `IsNull()`, `IsBoolean()`, `IsNumber()`, `IsString()`, `IsArray()`, `IsObject()`로 타입을 확인할 수 있습니다.

### 직렬화 및 파일 저장

```cpp
std::string compact = root.ToString();      // 한 줄 압축 출력
std::string pretty = root.ToString(2);       // 들여쓰기 2칸으로 예쁘게 출력

root.SaveToFile("output.json");       // 기본 들여쓰기 2칸으로 저장
root.SaveToFile("output.json", -1);   // 압축 형식으로 저장 (indent < 0)
```

- `SaveToFile`은 파일을 열 수 없거나 쓰기에 실패하면 `std::runtime_error`를 던집니다.

### 테스트 (`crud/JsonTests.cpp`)

- `JsonParseTest` / `JsonParseErrorTest`: object/array/nested 구조, 이스케이프·유니코드 문자열, 숫자 포맷(정수/음수/소수/지수) 파싱과, 잘못된 JSON 입력 시 `json::ParseException`(line/column 포함) 발생 검증
- `JsonAccessTest`: `operator[]`의 object 자동 생성(auto-vivify), 존재하지 않는 키에 대한 `const` 접근 예외, 타입 불일치 시 `AsXxx()` 예외, 배열 `PushBack`/인덱스 접근
- `JsonSerializeTest`: compact/pretty 직렬화 및 재파싱 라운드트립, 빈 array/object 직렬화
- `JsonFileIoTest`: `SaveToFile` → `ParseFile` 라운드트립, 존재하지 않는 파일 파싱 시 예외 검증 (`::testing::TempDir()` 사용)
- `JsonMockIntegrationTest`: `MOCK_METHOD`/`EXPECT_CALL`로 직렬화된 JSON을 전달받는 소비자(`JsonSink`)와의 상호작용을 검증하는 gmock 시나리오 (단일 호출 검증, `InSequence`를 이용한 배치 순서 검증)

## 퀵정렬 라이브러리 (`crud/QuickSort.h`)

헤더 전용 템플릿 라이브러리로, `sortlib` 네임스페이스에 `std::sort`처럼 랜덤 액세스 반복자 범위를 그 자리에서(in-place) 정렬하는 `QuickSort` 함수를 제공합니다. `std::vector`, `std::array`, 원시 포인터/배열 등 랜덤 액세스 반복자를 지원하는 어떤 컨테이너에도 사용할 수 있습니다.

```cpp
#include "QuickSort.h"

std::vector<int> v{5, 3, 1, 4, 2};
sortlib::QuickSort(v.begin(), v.end());                       // operator< 사용, 오름차순
sortlib::QuickSort(v.begin(), v.end(), std::greater<int>());  // 사용자 정의 비교자, 내림차순
```

- median-of-three 방식으로 피벗을 선택한 뒤 Hoare partition으로 분할하는 재귀 퀵정렬입니다. 이미 정렬된/역순 입력에서도 O(n²) 최악 케이스에 잘 빠지지 않도록 설계했습니다.
- 두 번째 인자로 비교자(`Compare`)를 넘기면 `std::greater<T>` 같은 커스텀 정렬 순서를 사용할 수 있습니다.
- 평균 시간복잡도 O(n log n), in-place 정렬(추가 메모리 O(log n) 스택 공간만 사용)이며, 퀵정렬 특성상 안정 정렬(stable sort)은 아닙니다.

### 테스트 (`crud/QuickSortTests.cpp`)

`QuickSortTest` 스위트에서 다음 시나리오를 검증합니다: 빈 범위, 단일 원소, 이미 정렬된 입력, 역순 입력, 무작위 입력, 중복 값 포함, 전부 동일한 값, 커스텀 비교자(내림차순), `std::string` 정렬, `std::array`/원시 배열 등 다양한 컨테이너, 그리고 `std::sort` 결과와의 비교 검증.

## CRUD 콘솔 애플리케이션

JSON 파일로 데이터를 관리하는 CRUD 콘솔 애플리케이션 본체입니다. 확장성(저장소 교체·모의 객체 대체 가능)과 가독성(계층별 책임 분리)을 위해 세 계층으로 나뉘어 있습니다.

- `crud/IRecordStore.h` — `Create`/`Find`(id 조회)/`FindByField`(키=값 조회)/`All`/`Update`/`Delete`를 정의하는 저장소 인터페이스. 레코드는 정수 `id` 필드를 가진 `json::Value` object로 취급합니다. 레코드에서 `id`를 뽑아내는 공용 함수 `crudapp::RecordId(const json::Value&)`도 여기서 제공합니다.
- `crud/JsonRecordStore.h`, `crud/JsonRecordStore.cpp` — `IRecordStore`의 JSON 파일 기반 구현체. 모든 변경(Create/Update/Delete) 직후 전체 컬렉션을 파일에 저장하므로 파일이 항상 메모리 상태를 반영합니다. 손상된 항목(오브젝트가 아니거나 `id`가 없는 항목)은 건너뛰고, 파일 자체가 JSON 배열이 아니거나 파싱에 실패하면 빈 저장소로 시작합니다(예외로 앱이 죽지 않도록 방어).
- `crud/ConsoleApp.h`, `crud/ConsoleApp.cpp` — `IRecordStore&`(구체 클래스가 아닌 인터페이스)를 받아 동작하는 대화형 메뉴(Create/Read all/Read one/Update/Delete/Exit). 입출력 스트림을 생성자 인자로 주입받아(`std::istream&`, `std::ostream&`, 기본값 `std::cin`/`std::cout`) 테스트에서 `istringstream`/`ostringstream`으로 교체할 수 있습니다. 목록 조회 시 `sortlib::QuickSort`로 `id` 기준 정렬 후 출력합니다. "Read one" 메뉴는 정수만 입력하면 id로, `key=value` 형식으로 입력하면 해당 필드 값으로 검색합니다(일치하는 레코드가 여러 개면 모두 출력).

### 사용법

```cpp
#include "ConsoleApp.h"
#include "JsonRecordStore.h"

crudapp::JsonRecordStore store("data.json");
crudapp::ConsoleApp app(store);
app.Run();
```

- `crud.exe` 실행 시 기본적으로 `data.json`을 저장소로 사용하는 대화형 CRUD 메뉴가 실행됩니다.
- Create/Update에서 필드는 `key=value` 형식으로 한 줄씩 입력하고 빈 줄로 종료합니다(모든 값은 문자열로 저장됨).
- 레코드 수정/삭제는 `Create`가 반환한 정수 `id`로 지정합니다. 조회는 `id` 또는 `key=value`(예: `role=engineer`) 중 하나로 할 수 있습니다.

### 확장 포인트

- 새로운 저장소 백엔드(예: SQLite, 원격 API)가 필요하면 `IRecordStore`를 구현하기만 하면 되고, `ConsoleApp`은 변경할 필요가 없습니다.
- 새 메뉴 명령을 추가하려면 `ConsoleApp::Run()`의 분기와 `PrintMenu()`에 항목을 추가합니다.

### 데이터 영속성

데이터를 JSON 파일로 저장·불러오고 CRUD로 조작하는 계층은 다음과 같이 구성됩니다.

| 계층 | 파일 | 역할 |
| --- | --- | --- |
| 인터페이스 | `crud/IRecordStore.h` | 저장소가 구현해야 할 CRUD 계약을 정의. 구체적인 저장 방식(파일, DB 등)에 의존하지 않으므로 mock으로 대체 가능 |
| 구현체 | `crud/JsonRecordStore.h`, `.cpp` | `IRecordStore`를 JSON 파일로 구현. 정수 `id`를 가진 `json::Value` object들을 배열 하나로 관리 |
| 소비자 | `crud/ConsoleApp.h`, `.cpp` | `IRecordStore&`를 받아 대화형 메뉴로 CRUD를 노출 |
| 데모 | `crud/DataPersistenceDemo.h`, `.cpp` | 영속성 동작(재시작 후에도 데이터 유지)을 비대화형으로 시연 |

**CRUD 매핑**

| 연산 | `IRecordStore` 메서드 | 파일에 미치는 영향 |
| --- | --- | --- |
| Create | `Create(json::Value fields)` | 새 `id`를 할당해 배열에 추가하고, 전체 배열을 즉시 파일에 저장 |
| Read (전체) | `All()` | 파일 I/O 없음 (생성자 로드 시점에 메모리에 적재된 상태를 반환) |
| Read (단건/조건) | `Find(int id)`, `FindByField(key, value)` | 파일 I/O 없음, 메모리 배열 탐색 |
| Update | `Update(int id, const json::Value& fields)` | 대상 레코드에 필드를 병합(`id`는 보존)하고, 전체 배열을 즉시 파일에 저장 |
| Delete | `Delete(int id)` | 대상 레코드를 배열에서 제거하고, 전체 배열을 즉시 파일에 저장 |

**저장 방식 (write-through)**

- 생성자(`JsonRecordStore(filePath)`)가 파일을 열어 `json::Value::ParseFile`로 파싱하고, 각 원소를 검사해 object이면서 정수 `id`를 가진 항목만 메모리 배열(`records_`)에 적재합니다. 파일이 없거나, 파싱에 실패하거나, 최상위 값이 배열이 아니면 빈 저장소로 시작합니다(예외로 앱이 죽지 않도록 방어).
- `Create`/`Update`/`Delete`는 메모리 상태를 바꾼 직후 `Save()`를 호출해 배열 전체를 `json::Value::SaveToFile`로 다시 직렬화합니다. 따라서 파일은 항상 메모리 상태와 동일하며, 별도의 "저장" 명령이 필요 없습니다.
- `nextId_`는 로드 시 기존 레코드 중 최댓값 `id` + 1로 초기화되어, 앱을 재시작해도 `id` 채번이 이어집니다.

**영속성 데모 실행 방법**

```
crud.exe --demo
```

`crud/DataPersistenceDemo.cpp`가 전용 파일(`demo_data.json`, 실행마다 초기화)을 사용해 다음을 자동으로 수행하고 각 단계를 콘솔에 출력합니다.

1. 빈 저장소로 시작
2. 레코드 2건 Create → 메모리 상태 출력
3. **같은 파일 경로로 새 `JsonRecordStore` 인스턴스를 생성**해 "앱 재시작"을 시뮬레이션하고, 데이터가 그대로 남아있음을 확인
4. Update 1건, Delete 1건 수행 → 반영 결과 출력
5. 디스크에 저장된 JSON 파일의 raw 내용 출력

**제약사항**

- **트랜잭션/동시성 미지원**: 단일 프로세스가 단일 파일을 독점적으로 사용한다고 가정합니다. 여러 프로세스가 동시에 같은 파일을 열면 마지막에 저장한 쪽의 내용으로 덮어써집니다.
- **전체 재직렬화 방식**: 변경 1건마다 컬렉션 전체를 다시 직렬화해 파일에 쓰므로, 레코드 수가 많아질수록 매 변경의 비용이 커집니다(부분 업데이트 없음).
- **스키마 검증 없음**: 레코드 필드는 `ConsoleApp`에서 `key=value` 형태의 문자열로만 입력되며, 타입이나 필수 필드를 강제하지 않습니다.
- **손상 데이터에 대한 방어적 무시**: object가 아니거나 `id`가 없는 항목은 로드 시 조용히 건너뛰고, 파일 자체가 손상되었거나 배열이 아니면 빈 저장소로 시작합니다. 손상 여부를 사용자에게 알리지는 않습니다.
- **백업/버전 관리 없음**: 매 저장이 기존 파일 내용을 덮어쓰므로, 이전 상태로 되돌릴 방법은 별도로 제공되지 않습니다.

**테스트 (`crud/DataPersistenceDemoTests.cpp`, Debug 전용)**

`RunDataPersistenceDemo`는 두 번째 인자로 파일 경로를 받을 수 있어(`filePath`, 기본값 `"demo_data.json"`), 테스트에서 `::testing::TempDir()` 기반 경로를 넘겨 격리된 상태로 검증합니다. `DataPersistenceDemoTest` 스위트가 확인하는 내용: 최초 실행 시 빈 저장소로 시작하는지, Create한 레코드가 재시작 시뮬레이션 후에도 남아있는지, Update/Delete 결과가 디스크 파일에 정확히 반영되는지, 마지막 단계에서 raw 파일 내용이 출력에 포함되는지, 그리고 같은 경로로 반복 실행해도(파일을 초기화하므로) 동일한 결과가 나오는지.

### 테스트 (`crud/JsonRecordStoreTests.cpp`, `crud/ConsoleAppTests.cpp`)

- `JsonRecordStoreTest`: id 자동 증가, `Find`/`FindByField`/`Update`/`Delete` 정상 동작, `id`를 덮어쓰지 않는 병합, 인스턴스 재시작 후 영속성 및 id 이어받기, 저장된 파일이 실제로 파싱 가능한지 검증, 손상된 항목/파싱 불가 파일/배열이 아닌 최상위 값에 대한 방어적 동작, object가 아닌 `fields`로 `Update` 호출 시 무해하게 무시되는지, 하나만 수정해도 다른 필드는 그대로인지, 하나를 삭제해도 다른 레코드는 메모리·파일 양쪽에서 온전한지, 범위를 벗어난 id에 대한 `Find`/`Delete`
- `ConsoleAppTest`: Create→Read all 흐름, 존재하지 않는 레코드 조회/수정/삭제, `key=value`로 필드 검색(일치/불일치), 정렬된 목록 출력, 잘못된 메뉴 입력, 숫자가 아닌 id 입력, 잘못된 `key=value` 줄 무시, 빈 필드로도 레코드가 생성되는지, 그리고 `MockRecordStore`(`IRecordStore`를 목으로 구현)를 이용해 `ConsoleApp`이 저장소를 정확한 인자로 호출하는지(`MOCK_METHOD`/`EXPECT_CALL`) 검증하는 gmock 시나리오

### 코드/테스트 에이전트 검증

이 CRUD 애플리케이션은 [`agents/code-expert.md`](agents/code-expert.md), [`agents/test-expert.md`](agents/test-expert.md)의 관점으로 두 차례 리뷰를 거쳤습니다.

1. 1차 리뷰(구현 직후): `IRecordStore` 인터페이스 분리를 통한 확장성 확보, 중복된 id 조회 로직 통합, 손상된 데이터 파일에 대한 방어적 로딩, id 입력 파싱 엄격화, 손상 파일/mock 기반 테스트 보강 등을 반영.
2. 2차 리뷰(요구사항 스펙 대조): 강의 자료에 명시된 CRUD 요구사항("Read: 전체 목록 보기 및 특정 ID/키 값으로 검색")과 실제 구현을 줄 단위로 대조한 결과, id로만 조회 가능하고 임의의 필드(키) 값으로는 검색할 수 없다는 실제 기능 gap을 발견하여 `IRecordStore::FindByField`/`JsonRecordStore::FindByField`를 추가하고 "Read one" 메뉴가 `id` 또는 `key=value` 둘 다 지원하도록 확장. 아울러 삭제가 다른 레코드를 훼손하지 않는지(다중 레코드 삭제 안전성), 수정이 지정한 필드만 바꾸고 나머지는 그대로 두는지에 대한 테스트도 보강.

## 테스트 실행

NuGet 패키지 `gmock` 1.11.0(gtest 포함, `crud/packages.config`, `crud/crud.vcxproj`의 `ExtensionTargets`로 연결)을 사용합니다. 별도 테스트 프로젝트 없이 같은 `crud` 프로젝트 안에 테스트를 두고, `crud.exe --test`로 실행하면 `RUN_ALL_TESTS()`가 호출되어 모든 테스트(Json/QuickSort/JsonRecordStore/ConsoleApp/DataPersistenceDemo)가 한 번에 실행됩니다. 인자 없이 `crud.exe`를 실행하면 대화형 CRUD 콘솔 앱이 시작되고, `crud.exe --demo`로 실행하면 데이터 영속성 데모가 실행됩니다.

**Debug 전용 테스트 빌드**: `*Tests.cpp` 파일들과 이들이 끌어오는 gtest/gmock 소스(`gtest-all.cc`, `gmock-all.cc`)는 `crud.vcxproj`에서 Release 구성일 때 `ExcludedFromBuild`로 컴파일에서 제외됩니다. `main.cpp`의 `--test` 처리와 `<gmock/gmock.h>`/`<gtest/gtest.h>` include도 `#ifdef _DEBUG`로 감싸져 있어, Release 바이너리는 gtest/gmock에 전혀 의존하지 않고 CRUD 콘솔 앱과 `--demo`만 포함합니다. 테스트는 Debug 빌드(`crud.exe --test`)에서만 실행할 수 있습니다.

**Visual Studio에서 F5로 바로 테스트 확인하기**: `crud.vcxproj.user`는 커밋되지 않는 사용자별 설정 파일(`.gitignore`의 `*.vcxproj.user`)이라, 새로 클론했거나 이 설정이 없는 환경에서는 F5(디버그 시작)를 누르면 명령줄 인자가 비어 있어 대화형 콘솔 메뉴가 뜹니다. Debug 구성에서 F5로 바로 테스트를 돌리려면 프로젝트 속성 → 디버깅(Debugging) → 명령 인수(Command Arguments)에 `--test`를 Debug|Win32, Debug|x64 두 구성에 입력하면 됩니다(Release 구성은 비워 두어 원래 콘솔 앱이 실행되게 합니다).



## commit 정책
제목 해더에 아래 키워드로 시작한다.
[test] : 테스트 코드 반영
[fix] : bug 수정
[feat] : 기능 구현
[refac] : 코드 리팩토링
[xxx] : 기타 정보 표시

commit message 는 아래 내용으로 한다.
- 요약 정보
- 빌드결과
- 테스트 결과 표시