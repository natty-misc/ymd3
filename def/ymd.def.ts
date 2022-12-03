/**
 * Type definitions for the YMD scripting API.
 * */

class YMD
{
    static readonly inputURL: String;
    static readonly retrieve: (url: String) => String;
    static readonly getVersion: () => String;
    static readonly log: (message: String) => void;

    static videoURL: String;
    static videoName: String;
    static videoAuthor: String | undefined;
    static downloadURL: String;
}
