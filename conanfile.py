from conan import ConanFile  # type: ignore[import-not-found]
from conan.tools.cmake import cmake_layout  # type: ignore[import-not-found]


class BcmdRecipe(ConanFile):
    name = "bcmd"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("grpc/1.65.0")
        self.requires("ftxui/5.0.0")
        self.requires("cli11/2.4.2")
        self.requires("spdlog/1.14.1")
        self.requires("nlohmann_json/3.11.3")
        self.requires("stduuid/1.2.3")

    def build_requirements(self):
        self.test_requires("catch2/3.7.1")
        self.tool_requires("cmake/[>=3.27]")

    def layout(self):
        cmake_layout(self)
