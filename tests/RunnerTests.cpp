#include <catch.hpp>

#include <QProcess>

QString runnerPath = "../ps3run/ps3run";

TEST_CASE("simple_printf") {
    QProcess proc;
    proc.start(runnerPath, QStringList() << "./binaries/simple_printf/a.elf");
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "some output\n" );
}

TEST_CASE("bubblesort") {
    QProcess proc;
    auto args = QStringList() << "./binaries/bubblesort/a.elf" 
                              << "5" << "17" << "30" << "-1" << "20" << "12" << "100" << "0";
    proc.start(runnerPath, args);
    proc.waitForFinished();
    auto output = QString(proc.readAll()).toStdString();
    REQUIRE( output == "args: 5 17 30 -1 20 12 100 0 \nsorted: -1 0 5 12 17 20 30 100 \n" );
}