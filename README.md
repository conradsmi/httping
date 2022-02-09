# HTTPing

## Important Notice

This was written in about half a day for the 2020 Cloudflare SWE Internship OA. It would have taken *much, much* longer if I didn't have snippets from prior projects readily available. This was just the second, "optional" half of the "assessment". After submitting both halfs of the assessment with what I assumed to be the proper fulfillment of specifications/expectations, I was lead along by recruiters telling me that I would get a response within two weeks and then was subsequently ghosted. If you are a recruiter (for *any* company) and are reading this for some reason, know that this is absolutely __not__ the way you should be recruiting sophomores for 10-week internships. I will __never__ recommend anyone apply to Cloudflare for __any__ type of role after this experience, because if they're expecting this level of work from sophomores for an automatic assessment, before they are put into contact with anyone else from the company, what could they possibly expect from applicants to full-time roles?

## Original README

A simple command-line HTTP GET Request tool written entirely in C.

### How it works

*HTTPing* takes a url and "profile" (# of GET requests to make) and generates the specified number of GET requests, printing the response directly to the console. Additionally, a statistics summary is featured at the end of each execution.

__NOTE:__ This will not work in Windows. Please use a Unix-based operating system or Windows Subsystem for Linux (WSL) to execute this program.

### Installation

Installation is trivial, since HTTPing is contained within a single file. Firstly, ensure that `make` and its prerequisites are installed. Next, execute the following commands to build and install HTTPing:

    `make` or `make httping`
    `make install`
    `make clean`

The binary file will be moved to /usr/local/bin. To uninstall, execute:

    `make uninstall`

### Usage

HTTPing takes two required arguments: -u (or --url) and -p (or --profile). Specific information about the arguments is listed below.

* -h, --help: prints this help message and ends execution.
* -u, --url: the URL to send GET requests to. Can be of the following forms:
    www.google.com, www.google.com:80/index.html, www.google.com/, www.google.com:80
    If the port and page is not specified, port 80 will be used and page '/' will be used by default.
    (Note that the preceding http:// or https:// should not be included)
* -p, --profile: the number of GET requests to send. Each request is preceded by a one second break.

Example usage:

`httping --url localhost:8000/links --profile 4`

A sample of the output for this example and one that pings www.duckduckgo.com can be found in the ZIP file. Note that these do not show a profile greater than 1, as each ping produces a lengthy response.
