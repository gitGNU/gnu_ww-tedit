void strlwr(char *d)
{
  if (!d)
    return;
    
  while (*d)
  {
    if (*d >= 'A' && *d <= 'Z')
      *d = *d + ('a' - 'A');
    ++d;
  }
}

