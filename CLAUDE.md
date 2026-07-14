# crud

C++20 Visual Studio 프로젝트. `crud/` 폴더에 JSON 파싱 및 파일 저장을 지원하는 자체 구현 JSON 라이브러리(`json::Value`)가 포함되어 있습니다. 외부 의존성 없이 표준 라이브러리만 사용합니다.

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
