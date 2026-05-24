from conan import ConanFile  # type: ignore[import-not-found]


class BcmdRecipe(ConanFile):
    name = "bcmd"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("grpc/1.78.1")
        self.requires("protobuf/6.33.5", override=True)
        self.requires("ftxui/6.1.9")
        self.requires("cli11/2.6.2")
        self.requires("spdlog/1.17.0")
        self.requires("stduuid/1.2.3")

    def build_requirements(self):
        self.test_requires("catch2/3.7.1")
        self.tool_requires("cmake/[>=3.27]")
        self.tool_requires("protobuf/<host_version>")

    def layout(self):
        self.folders.source = "."
        self.folders.generators = "generators"
        self.folders.build = "."
