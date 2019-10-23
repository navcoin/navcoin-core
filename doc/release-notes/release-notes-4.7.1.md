# NavCoin v4.7.1 Release Notes

## Merged PRs

* [`Pull Request 509`](https://github.com/navcoin/navcoin-core/pull/509) [`Commit 40eac7ab`](https://github.com/navcoin/navcoin-core/commit/58e38079d7d854a6b02ebb228f06244140eac7ab) Updated some NULL -> nullptr
* [`Pull Request 525`](https://github.com/navcoin/navcoin-core/pull/525) [`Commit 8ed8cf87`](https://github.com/navcoin/navcoin-core/commit/2e6aa1b3e598d3a443343c480bdbf6b88ed8cf87) Replaced all BOOST_FOREACH calls with new for() syntax (c++11)
* [`Pull Request 554`](https://github.com/navcoin/navcoin-core/pull/554) [`Commit 82523147`](https://github.com/navcoin/navcoin-core/commit/0a8c872a60169de4f6b57b83dab9b39382523147) Replaced Q_FOREACH with for() from c++11 standard
* [`Pull Request 560`](https://github.com/navcoin/navcoin-core/pull/560) [`Commit 762c3ffd`](https://github.com/navcoin/navcoin-core/commit/64f8cd453f4bdda04f4a718cb026d8a8762c3ffd) Set pindexDelete to nullptr on intialize
* [`Pull Request 578`](https://github.com/navcoin/navcoin-core/pull/578) [`Commit c464383b`](https://github.com/navcoin/navcoin-core/commit/da5377e89a25cfa54a52768393630134c464383b) Sort available coins for staking by coin age descending
* [`Pull Request 587`](https://github.com/navcoin/navcoin-core/pull/587) [`Commit ff6f543e`](https://github.com/navcoin/navcoin-core/commit/49f74084cf9eed8d8e7c46707d836b82ff6f543e) Add support for raw script addresses
* [`Pull Request 603`](https://github.com/navcoin/navcoin-core/pull/603) [`Commit f755e298`](https://github.com/navcoin/navcoin-core/commit/6fe0683ba99ce912da4d9181094ab4baf755e298) Updated depends to use winssl for windows and darwinssl for osx
* [`Pull Request 605`](https://github.com/navcoin/navcoin-core/pull/605) [`Commit 5e0af830`](https://github.com/navcoin/navcoin-core/commit/0b8cb5dd81186fcd54860fe7c25f2cac5e0af830) Staking reward setup GUI
* [`Pull Request 608`](https://github.com/navcoin/navcoin-core/pull/608) [`Commit 07d49bf2`](https://github.com/navcoin/navcoin-core/commit/e688c6ed6a1da2734aa89b41ae16051807d49bf2) Fix CFund DB read after nullified entry
* [`Pull Request 611`](https://github.com/navcoin/navcoin-core/pull/611) [`Commit 30401b32`](https://github.com/navcoin/navcoin-core/commit/f7b1c6304200052418c66e8f242ddf8c30401b32) Fixed error.log loading in Debug Window (Windows)
* [`Pull Request 612`](https://github.com/navcoin/navcoin-core/pull/612) [`Commit d5cfa467`](https://github.com/navcoin/navcoin-core/commit/902970adfdd5ce0e54e54bfa7545edfad5cfa467) Fix random RPC tests failing
