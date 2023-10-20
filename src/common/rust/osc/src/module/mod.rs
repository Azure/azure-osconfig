use std::{
    cmp::Ordering,
    error::Error,
    fmt::{self, Display},
};

use serde::{Deserialize, Serialize, de::DeserializeOwned};
use serde_repr::{Deserialize_repr, Serialize_repr};

pub mod bind;

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "PascalCase")]
pub struct Meta {
    pub name: String,
    pub description: Option<String>,
    pub manufacturer: Option<String>,
    #[serde(flatten)]
    pub version: Version,
    pub components: Vec<String>,
    pub user_account: UserAccount,
    pub lifetime: Lifetime,
}

impl Display for Meta {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let str = serde_json::to_string(&self).unwrap();
        write!(f, "{}", str)
    }
}

#[derive(Default, Debug, Eq, Serialize, Deserialize, PartialEq)]
pub struct Version {
    #[serde(rename = "VersionMajor", default)]
    pub major: u32,
    #[serde(rename = "VersionMinor", default)]
    pub minor: u32,
    #[serde(rename = "VersionPatch", default)]
    pub patch: u32,
    #[serde(rename = "VersionTweak", default)]
    pub tweak: u32,
}

impl Ord for Version {
    fn cmp(&self, other: &Self) -> Ordering {
        match self.major.cmp(&other.major) {
            Ordering::Equal => match self.minor.cmp(&other.minor) {
                Ordering::Equal => match self.patch.cmp(&other.patch) {
                    Ordering::Equal => self.tweak.cmp(&other.tweak),
                    ordering => ordering,
                },
                ordering => ordering,
            },
            ordering => ordering,
        }
    }
}

impl PartialOrd for Version {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl From<&str> for Version {
    fn from(version: &str) -> Self {
        let mut split = version.split('.');

        let major = split.next().unwrap_or("0").parse().unwrap_or(0);
        let minor = split.next().unwrap_or("0").parse().unwrap_or(0);
        let patch = split.next().unwrap_or("0").parse().unwrap_or(0);
        let tweak = split.next().unwrap_or("0").parse().unwrap_or(0);

        Self {
            major,
            minor,
            patch,
            tweak,
        }
    }
}

impl fmt::Display for Version {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "v{}.{}.{}.{}",
            self.major, self.minor, self.patch, self.tweak
        )
    }
}

/// The lifetime of a module determines how long the module will be kept loaded. Short lifetime
/// modules will be loaded/unloaded for each request. Long lifetime modules will be kept loaded
/// until the platform is restarted.
#[derive(Debug, PartialEq, Serialize_repr, Deserialize_repr)]
#[serde(rename_all = "lowercase")]
#[repr(u8)]
pub enum Lifetime {
    Short = 1,
    Long = 2,
}

/// The UID of the user account that the module will be run with. Root (0) is default.
/// UIDs can be found in `/etc/passwd`.
///
/// _Note: UIDs can change/be moved._
#[derive(Debug, PartialEq)]
pub enum UserAccount {
    Root,
    User(u32),
}

impl From<i32> for UserAccount {
    fn from(value: i32) -> Self {
        match value {
            0 => Self::Root,
            value => Self::User(value as u32),
        }
    }
}

impl Serialize for UserAccount {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::ser::Serializer,
    {
        match self {
            Self::Root => serializer.serialize_i32(0),
            Self::User(uid) => serializer.serialize_u32(*uid),
        }
    }
}

impl<'de> Deserialize<'de> for UserAccount {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::de::Deserializer<'de>,
    {
        Ok(i32::deserialize(deserializer)?.into())
    }
}

pub struct Value(serde_json::Value);

/// Error type for errors that occur during property resolution.
///
/// Property errors are represented by a human-readable error message
/// which can be displayed in log traces.
pub struct PropertyError {
    message: String,
}

impl PropertyError {
    pub fn new(message: String) -> Self {
        Self { message }
    }
}

impl fmt::Display for PropertyError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

/// The result of resolving the value of a field of type `T`
pub type PropertyResult<T> = Result<T, PropertyError>;

/// Custom error handling trait to enable error types other than [`PropertyError`]
/// to be specified as a return value for property resolvers.
pub trait IntoPropertyError {
    /// Performs the custom conversion into a [`PropertyError`].
    fn into_prop_error(self) -> PropertyError;
}

impl<T> IntoPropertyError for T
where
    T: Error,
{
    fn into_prop_error(self) -> PropertyError {
        PropertyError::new(self.to_string())
    }
}

/// Custom trait for converting custom result types into [`PropertyResult`].
pub trait IntoPropertyResult<T> {
    #[doc(hidden)]
    fn into_result(self) -> PropertyResult<T>;
}

impl<T> IntoPropertyResult<T> for T
where
    T: Serialize,
{
    fn into_result(self) -> PropertyResult<T> {
        Ok(self)
    }
}

impl<T, E> IntoPropertyResult<T> for Result<T, E>
where
    T: IntoPropertyResult<T>,
    E: IntoPropertyError,
{
    fn into_result(self) -> PropertyResult<T> {
        self.map_err(E::into_prop_error)
    }
}

/// Custom trait for converting custom result types into payload strings.
pub trait IntoPayload {
    fn into_payload(self) -> PropertyResult<String>;
}

impl<T> IntoPayload for PropertyResult<T>
where
    T: Serialize,
{
    fn into_payload(self) -> PropertyResult<String> {
        match self {
            Ok(value) => Ok(serde_json::to_string(&value).unwrap()),
            Err(err) => Err(err),
        }
    }
}

/// Custom trait for converting payloads into custom argument types.
pub trait IntoPropertyArgument<T> {
    #[doc(hidden)]
    fn into_arg(self) -> T;
}

impl<T> IntoPropertyArgument<T> for String
where
    T: DeserializeOwned,
{
    fn into_arg(self) -> T {
        serde_json::from_str::<T>(&self).unwrap()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_version() {
        let version = Version::from("1.2.3.4");

        assert_eq!(version.major, 1);
        assert_eq!(version.minor, 2);
        assert_eq!(version.patch, 3);
        assert_eq!(version.tweak, 4);
    }

    fn test_version_cmp(a: &str, b: &str, ordering: Ordering) {
        let a = Version::from(a);
        let b = Version::from(b);

        assert_eq!(a.cmp(&b), ordering);
    }

    #[test]
    fn test_version_cmp_major() {
        test_version_cmp("1.0.0.0", "1.0.0.0", Ordering::Equal);
        test_version_cmp("0.1.0.0", "0.1.0.0", Ordering::Equal);
        test_version_cmp("0.0.1.0", "0.0.1.0", Ordering::Equal);
        test_version_cmp("0.0.0.1", "0.0.0.1", Ordering::Equal);

        test_version_cmp("1.0.0.0", "1.0.0.1", Ordering::Less);
        test_version_cmp("1.0.0.0", "1.0.1.0", Ordering::Less);
        test_version_cmp("1.0.0.0", "1.1.0.0", Ordering::Less);

        test_version_cmp("1.0.0.1", "1.0.0.0", Ordering::Greater);
        test_version_cmp("1.0.1.0", "1.0.0.0", Ordering::Greater);
        test_version_cmp("1.1.0.0", "1.0.0.0", Ordering::Greater);
    }
}
