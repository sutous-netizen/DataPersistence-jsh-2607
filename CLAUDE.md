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

### 예제 (`crud/main.cpp`)

`main.cpp`는 object/array를 구성하고, 콘솔에 출력하고, `output.json`으로 저장한 뒤 다시 읽어들이는 전체 흐름을 보여주는 데모입니다. 데모 실행 후 이어서 gtest/gmock 테스트가 실행됩니다.

## 테스트 (`crud/JsonTests.cpp`)

NuGet 패키지 `gmock` 1.11.0(gtest 포함, `crud/packages.config`, `crud/crud.vcxproj`의 `ExtensionTargets`로 연결)을 사용합니다. 별도 테스트 프로젝트 없이 `crud` 프로젝트 안에서 `main()`이 데모 실행 후 `RUN_ALL_TESTS()`를 호출하는 구조입니다.

- `JsonParseTest` / `JsonParseErrorTest`: object/array/nested 구조, 이스케이프·유니코드 문자열, 숫자 포맷(정수/음수/소수/지수) 파싱과, 잘못된 JSON 입력 시 `json::ParseException`(line/column 포함) 발생 검증
- `JsonAccessTest`: `operator[]`의 object 자동 생성(auto-vivify), 존재하지 않는 키에 대한 `const` 접근 예외, 타입 불일치 시 `AsXxx()` 예외, 배열 `PushBack`/인덱스 접근
- `JsonSerializeTest`: compact/pretty 직렬화 및 재파싱 라운드트립, 빈 array/object 직렬화
- `JsonFileIoTest`: `SaveToFile` → `ParseFile` 라운드트립, 존재하지 않는 파일 파싱 시 예외 검증 (`::testing::TempDir()` 사용)
- `JsonMockIntegrationTest`: `MOCK_METHOD`/`EXPECT_CALL`로 직렬화된 JSON을 전달받는 소비자(`JsonSink`)와의 상호작용을 검증하는 gmock 시나리오 (단일 호출 검증, `InSequence`를 이용한 배치 순서 검증)

빌드 후 `crud.exe`를 실행하면 데모 출력과 함께 테스트 결과가 콘솔에 출력됩니다.

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

`QuickSortTest` 스위트에서 다음 시나리오를 검증합니다: 빈 범위, 단일 원소, 이미 정렬된 입력, 역순 입력, 무작위 입력, 중복 값 포함, 전부 동일한 값, 커스텀 비교자(내림차순), `std::string` 정렬, `std::array`/원시 배열 등 다양한 컨테이너, 그리고 `std::sort` 결과와의 비교 검증. 총 12개 테스트이며 위의 JSON 테스트와 함께 `crud.exe` 실행 시 자동으로 실행됩니다.
